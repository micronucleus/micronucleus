

#ifndef DEVICE_HARDWARE
#define DEVICE_HARDWARE 1

#ifndef PCINT_vect_num
#  define PCINT_vect_num	2
#endif

#define USB_CFG_IOPORTNAME      B
#define USB_CFG_DMINUS_BIT      6
#define USB_CFG_DPLUS_BIT       3

/* use pin-change interrupt on D+ */
#define USB_INTR_CFG            PCMSK1
#define USB_INTR_CFG_SET        _BV(PCINT11)
#define USB_INTR_CFG_CLR        0

#define USB_INTR_ENABLE         GIMSK
#define USB_INTR_ENABLE_BIT     PCIE0

#define USB_INTR_PENDING        GIFR
#define USB_INTR_PENDING_BIT    PCIF
#define USB_INTR_VECTOR         PCINT_vect
#define USB_INTR_VECTOR_NUM     PCINT_vect_num

#define WITH_CRYSTAL	1

/*
 * The device has a LED and it is connected to PB1. This
 * PIN is also DO on the ISP, which gives you blinking
 * while flashing the device over the ISP.
 */
#define LED_PORT	B
#define LED_PIN		1

#endif
