/* Name: usbdrv.c
 * Project: V-USB, virtual USB port for Atmel's(r) AVR(r) microcontrollers
 * Author: Christian Starkjohann
 * Creation Date: 2004-12-29
 * Tabsize: 4
 *

 * Copyright: (c) 2005 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 */

/* This copy of usbdrv.c was optimized to reduce the memory footprint with micronucleus V2
 *
 * Changes:
 *     a) Replies to USB SETUP IN Packets are now only possible from Flash
 *       * Commented out routines to copy from SRAM
 *       * remove msgflag variable and all handling involving it
 *
 *     b) Do not use preinitialized global variables to avoid having to initialize
 *        the data section.
 */

#include "usbdrv.h"
#include "oddebug.h"

/*
 General Description:
 This module implements the C-part of the USB driver. See usbdrv.h for a
 documentation of the entire driver.
 */

/* ------------------------------------------------------------------------- */

/* raw USB registers / interface to assembler code: */
uchar usbRxBuf[2 * USB_BUFSIZE]; /* raw RX buffer: PID, 8 bytes data, 2 bytes CRC */
uchar usbInputBufOffset; /* offset in usbRxBuf used for low level receiving */
uchar usbDeviceAddr; /* assigned during enumeration, defaults to 0 */
uchar usbNewDeviceAddr; /* device ID which should be set after status phase */
uchar usbConfiguration; /* currently selected configuration. Administered by driver, but not used */
volatile schar usbRxLen; /* = 0; number of bytes in usbRxBuf; 0 means free, -1 for flow control */
uchar usbCurrentTok; /* last token received or endpoint number for last OUT token if != 0 */
uchar usbRxToken; /* token for data we received; or endpont number for last OUT */
volatile uchar usbTxLen; /* number of bytes to transmit with next IN token or handshake token */
uchar usbTxBuf[USB_BUFSIZE];/* data to transmit with next IN, free if usbTxLen contains handshake token */

/* USB status registers / not shared with asm code */
usbMsgPtr_t usbMsgPtr; /* data to transmit next -- ROM or RAM address */
#if defined(STORE_CONFIGURATION_REPLY_IN_RAM)
uchar usbMsgPtrIsRAMAddress = 0; /* true if RAM address */
#endif
static usbMsgLen_t usbMsgLen; /* remaining number of bytes */

#define USB_FLG_USE_USER_RW     (1<<7)

/*
 optimizing hints:
 - do not post/pre inc/dec integer values in operations
 - assign value of USB_READ_FLASH() to register variables and don't use side effects in arg
 - use narrow scope for variables which should be in X/Y/Z register
 - assign char sized expressions to variables to force 8 bit arithmetics
 */

/* -------------------------- String Descriptors --------------------------- */

#if USB_CFG_DESCR_PROPS_STRINGS == 0

#if USB_CFG_DESCR_PROPS_STRING_0 == 0
#undef USB_CFG_DESCR_PROPS_STRING_0
#define USB_CFG_DESCR_PROPS_STRING_0    sizeof(usbDescriptorString0)
PROGMEM const char usbDescriptorString0[] = { /* language descriptor */
    4, /* sizeof(usbDescriptorString0): length of descriptor in bytes */
    3, /* descriptor type */
    0x09, 0x04, /* language index (0x0409 = US-English) */
};
#endif

#if USB_CFG_DESCR_PROPS_STRING_VENDOR == 0 && USB_CFG_VENDOR_NAME_LEN
#undef USB_CFG_DESCR_PROPS_STRING_VENDOR
#define USB_CFG_DESCR_PROPS_STRING_VENDOR   sizeof(usbDescriptorStringVendor)
PROGMEM const int usbDescriptorStringVendor[] = {
    USB_STRING_DESCRIPTOR_HEADER(USB_CFG_VENDOR_NAME_LEN),
    USB_CFG_VENDOR_NAME
};
#endif

#if USB_CFG_DESCR_PROPS_STRING_PRODUCT == 0 && USB_CFG_DEVICE_NAME_LEN
#undef USB_CFG_DESCR_PROPS_STRING_PRODUCT
#define USB_CFG_DESCR_PROPS_STRING_PRODUCT   sizeof(usbDescriptorStringDevice)
PROGMEM const int usbDescriptorStringDevice[] = {
    USB_STRING_DESCRIPTOR_HEADER(USB_CFG_DEVICE_NAME_LEN),
    USB_CFG_DEVICE_NAME
};
#endif

#if USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER == 0 && USB_CFG_SERIAL_NUMBER_LEN
#undef USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER
#define USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER    sizeof(usbDescriptorStringSerialNumber)
PROGMEM const int usbDescriptorStringSerialNumber[] = {
    USB_STRING_DESCRIPTOR_HEADER(USB_CFG_SERIAL_NUMBER_LEN),
    USB_CFG_SERIAL_NUMBER
};
#endif

#endif  /* USB_CFG_DESCR_PROPS_STRINGS == 0 */

/* --------------------------- Device Descriptor --------------------------- */

#if USB_CFG_DESCR_PROPS_DEVICE == 0
#undef USB_CFG_DESCR_PROPS_DEVICE
#define USB_CFG_DESCR_PROPS_DEVICE  sizeof(usbDescriptorDevice)
PROGMEM const char usbDescriptorDevice[] = { /* USB device descriptor */
18, /* sizeof(usbDescriptorDevice): length of descriptor in bytes */
USBDESCR_DEVICE, /* descriptor type */
0x10, 0x01, /* USB version supported */
USB_CFG_DEVICE_CLASS,
USB_CFG_DEVICE_SUBCLASS, 0, /* protocol */
8, /* max packet size */
/* the following two casts affect the first byte of the constant only, but
 * that's sufficient to avoid a warning with the default values.
 */
(char) USB_CFG_VENDOR_ID,/* 2 bytes */
(char) USB_CFG_DEVICE_ID,/* 2 bytes */
USB_CFG_DEVICE_VERSION, /* 2 bytes */
USB_CFG_DESCR_PROPS_STRING_VENDOR != 0 ? 1 : 0, /* manufacturer string index */
USB_CFG_DESCR_PROPS_STRING_PRODUCT != 0 ? 2 : 0, /* product string index */
USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER != 0 ? 3 : 0, /* serial number string index */
1, /* number of configurations */
};
#endif

/* ----------------------- Configuration Descriptor ------------------------ */

#if USB_CFG_DESCR_PROPS_CONFIGURATION == 0
#undef USB_CFG_DESCR_PROPS_CONFIGURATION
#define USB_CFG_DESCR_PROPS_CONFIGURATION   sizeof(usbDescriptorConfiguration)
PROGMEM const char usbDescriptorConfiguration[] = { /* USB configuration descriptor */
9, /* sizeof(usbDescriptorConfiguration): length of descriptor in bytes */
USBDESCR_CONFIG, /* descriptor type */
18, 0,
/* total length of data returned (including inlined descriptors) */
1, /* number of interfaces in this configuration */
1, /* index of this configuration */
0, /* configuration name string index */
(1 << 7), /* attributes */
USB_CFG_MAX_BUS_POWER / 2, /* max USB current in 2mA units */
/* interface descriptor follows inline: */
9, /* sizeof(usbDescrInterface): length of descriptor in bytes */
USBDESCR_INTERFACE, /* descriptor type */
0, /* index of this interface */
0, /* alternate setting for this interface */
0, /* endpoints excl 0: number of endpoint descriptors to follow */
USB_CFG_INTERFACE_CLASS,
USB_CFG_INTERFACE_SUBCLASS,
USB_CFG_INTERFACE_PROTOCOL, 0, /* string index for interface */
};
#endif

/* ------------------ utilities for code following below ------------------- */
#ifndef USB_RX_USER_HOOK
#define USB_RX_USER_HOOK(data, len)
#endif
#ifndef USB_SET_ADDRESS_HOOK
#define USB_SET_ADDRESS_HOOK()
#endif

/* ------------------------------------------------------------------------- */

/* We use if() instead of #if in the macro below because #if can't be used
 * in macros and the compiler optimizes constant conditions anyway.
 * This may cause problems with undefined symbols if compiled without
 * optimizing!
 */
// no ram descriptor possible here
#define GET_DESCRIPTOR(cfgProp, staticName)         \
    if(cfgProp){                                    \
            len = USB_PROP_LENGTH(cfgProp);         \
            usbMsgPtr = (usbMsgPtr_t)(staticName);  \
    }

/* usbDriverDescriptor() is used internally for all types of descriptors.
 */
static inline usbMsgLen_t usbDriverDescriptor(usbRequest_t *rq) {
    usbMsgLen_t len = 0;
    {
        uchar _cmd = rq->wValue.bytes[1];
        if (_cmd == (USBDESCR_DEVICE)) { /* 1 */
            GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_DEVICE, usbDescriptorDevice)
        } else if (_cmd == (USBDESCR_CONFIG)) { /* 2 */
            GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_CONFIGURATION, usbDescriptorConfiguration)
        } else if (_cmd == (USBDESCR_STRING)) { /* 3 */

            uchar _cmd = rq->wValue.bytes[0];
            if (_cmd == (0)) {
                GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_STRING_0, usbDescriptorString0)
            } else if (_cmd == (1)) {
                GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_STRING_VENDOR, usbDescriptorStringVendor)
            } else if (_cmd == (2)) {
                GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_STRING_PRODUCT, usbDescriptorStringDevice)
            } else if (_cmd == (3)) {
                GET_DESCRIPTOR(USB_CFG_DESCR_PROPS_STRING_SERIAL_NUMBER, usbDescriptorStringSerialNumber)
            }
        }
    }

    return len;
}

/* ------------------------------------------------------------------------- */

/* usbDriverSetup() is similar to usbFunctionSetup(), but it's used for
 * standard requests instead of class and custom requests.
 */
static inline usbMsgLen_t usbDriverSetup(usbRequest_t *rq) {
    usbMsgLen_t len = 0;
    uchar *dataPtr = usbTxBuf + 9; /* there are 2 bytes free space at the end of the buffer */
    uchar value = rq->wValue.bytes[0];
    dataPtr[0] = 0; /* default reply common to USBRQ_GET_STATUS and USBRQ_GET_INTERFACE */
    {
        uchar tRequest = rq->bRequest;
        if (tRequest == (USBRQ_GET_STATUS)) { /* 0 */
            dataPtr[1] = 0;
            len = 2;
        } else if (tRequest == (USBRQ_SET_ADDRESS)) { /* 5 */
            usbNewDeviceAddr = value;
            USB_SET_ADDRESS_HOOK();
        } else if (tRequest == (USBRQ_GET_DESCRIPTOR)) { /* 6 */
            len = usbDriverDescriptor(rq);
            goto skipMsgPtrAssignment;
        } else if (tRequest == (USBRQ_GET_CONFIGURATION)) { /* 8 */
            dataPtr = &usbConfiguration; /* send current configuration value */
            len = 1;
        } else if (tRequest == (USBRQ_SET_CONFIGURATION)) { /* 9 */
            usbConfiguration = value;
        } else if (tRequest == (USBRQ_GET_INTERFACE)) { /* 10 */
            len = 1;
        } else { /* 7=SET_DESCRIPTOR, 12=SYNC_FRAME */
            /* Should we add an optional hook here? */
        }
    }
    usbMsgPtr = (usbMsgPtr_t) dataPtr;
    skipMsgPtrAssignment: return len;
}

/* ------------------------------------------------------------------------- */

/* usbProcessRx() is called for every message received by the interrupt
 * routine. It distinguishes between SETUP and DATA packets and processes
 * them accordingly.
 */
static inline void usbProcessRx(uchar *data, uchar len) {
    usbRequest_t *rq = (void*) data; // data is constant value usbRxBuf + 1 and optimized out

    /* usbRxToken can be:
     * 0x2d 00101101 (USBPID_SETUP for setup data)
     * 0xe1 11100001 (USBPID_OUT: data phase of setup transfer)
     * 0...0x0f for OUT on endpoint X
     */
    DBG2(0x10 + (usbRxToken & 0xf), data, len + 2); /* SETUP=1d, SETUP-DATA=11, OUTx=1x */
    USB_RX_USER_HOOK(data, len)
    if (usbRxToken == (uchar) USBPID_SETUP) {
        if (len != 8) /* Setup size must be always 8 bytes. Ignore otherwise. */
            return;
        usbMsgLen_t replyLen;
        usbTxBuf[0] = USBPID_DATA0; /* initialize data toggling */
        usbTxLen = USBPID_NAK; /* abort pending transmit */
        uchar type = rq->bmRequestType & USBRQ_TYPE_MASK; // masking is essential here
        if (type != USBRQ_TYPE_STANDARD) { /* standard requests are handled by driver */
            replyLen = usbFunctionSetup(data); // for USBRQ_TYPE_CLASS or USBRQ_TYPE_VENDOR
        } else {
            replyLen = usbDriverSetup(rq);
        }
        if (sizeof(replyLen) < sizeof(rq->wLength.word)) { /* help compiler with optimizing */
            if (!rq->wLength.bytes[1] && replyLen > rq->wLength.bytes[0]) /* limit length to max */
                replyLen = rq->wLength.bytes[0];
        } else {
            if (replyLen > rq->wLength.word) /* limit length to max */
                replyLen = rq->wLength.word;
        }
        usbMsgLen = replyLen;
    }
}

/* ------------------------------------------------------------------------- */

/* This function is only called from usbBuildTxBlock for data handled
 * automatically by the driver (e.g. descriptor reads).
 */
static void usbDeviceRead(uchar *data, usbMsgLen_t len) {
    if (len > 0) { /* don't bother app with 0 sized reads */
        uchar i = len;
        usbMsgPtr_t r = usbMsgPtr;
#if defined(STORE_CONFIGURATION_REPLY_IN_RAM)
        if (usbMsgPtrIsRAMAddress) {
            // RAM data
            do {
                *data++ = *((uchar*) r);
                r++;
            } while (--i);
            usbMsgPtrIsRAMAddress = 0;
        } else {
#endif
            do {
                uchar c = USB_READ_FLASH(r); /* assign to char size variable to enforce byte ops */
                *data++ = c;
                r++;
            } while (--i);
#if defined(STORE_CONFIGURATION_REPLY_IN_RAM)
        }
#endif
        usbMsgPtr = r;
    }
}

/* ------------------------------------------------------------------------- */

/* usbBuildTxBlock() is called when we have data to transmit and the
 * interrupt routine's transmit buffer is empty.
 */
static inline void usbBuildTxBlock(void) {

    usbMsgLen_t wantLen = usbMsgLen;
    if (wantLen > 8) {
        wantLen = 8;
    }
    usbMsgLen -= wantLen;
    usbTxBuf[0] ^= USBPID_DATA0 ^ USBPID_DATA1; /* DATA toggling */
    usbDeviceRead(usbTxBuf + 1, wantLen);
    usbCrc16Append(&usbTxBuf[1], wantLen);
    wantLen += 4; /* length including sync byte */
    if (wantLen < 12) { /* a partial package identifies end of message */
        usbMsgLen = USB_NO_MSG;
    }

    usbTxLen = wantLen;
    DBG2(0x20, usbTxBuf, len-1);
}

/* ------------------------------------------------------------------------- */

#ifdef USB_RESET_HOOK
static inline void usbHandleResetHook(uchar notResetState)
{

    static uchar wasReset;
    uchar isReset = !notResetState;

    if(wasReset != isReset) {
        USB_RESET_HOOK(isReset);
        wasReset = isReset;
    }
}
#else
//    notResetState = notResetState;  // avoid compiler warning -> leads to another warning :-(
#endif
/* ------------------------------------------------------------------------- */
// Replaced for micronucleus V2
//USB_PUBLIC void usbPoll(void)
//{
//schar   len;
//uchar   i;
//
//    len = usbRxLen - 3;
//    if(len >= 0){
///* We could check CRC16 here -- but ACK has already been sent anyway. If you
// * need data integrity checks with this driver, check the CRC in your app
// * code and report errors back to the host. Since the ACK was already sent,
// * retries must be handled on application level.
// * unsigned crc = usbCrc16(buffer + 1, usbRxLen - 3);
// */
//        usbProcessRx(usbRxBuf + USB_BUFSIZE + 1 - usbInputBufOffset, len);
//        usbRxLen = 0;       /* mark rx buffer as available */
//    }
//    if(usbTxLen & 0x10){    /* transmit system idle */
//        if(usbMsgLen != USB_NO_MSG){    /* transmit data pending? */
//            usbBuildTxBlock();
//        }
//    }
//    for(i = 20; i > 0; i--){
//        uchar usbLineStatus = USBIN & USBMASK;
//        if(usbLineStatus != 0)  /* SE0 has ended */
//            goto isNotReset;
//    }
//    /* RESET condition, called multiple times during reset */
//    usbNewDeviceAddr = 0;
//    usbDeviceAddr = 0;
//    usbResetStall();
//    DBG1(0xff, 0, 0);
//isNotReset:
//    usbHandleResetHook(i);
//}
/* ------------------------------------------------------------------------- */

USB_PUBLIC void usbInit(void) {
    usbTxLen = USBPID_NAK;
    usbMsgLen = USB_NO_MSG;

#if USB_INTR_CFG_SET != 0
    USB_INTR_CFG |= USB_INTR_CFG_SET;
#endif
#if USB_INTR_CFG_CLR != 0
    USB_INTR_CFG &= ~(USB_INTR_CFG_CLR);
#endif
#if (defined(GIMSK) || defined(EIMSK)) // GICR contains other bits, which must be kept
    USB_INTR_ENABLE |= (1 << USB_INTR_ENABLE_BIT);
#else
    USB_INTR_ENABLE = (1 << USB_INTR_ENABLE_BIT); // We only want one interrupt to be enabled, so keeping the other bits makes no sense.
#endif
}

/* ------------------------------------------------------------------------- */
