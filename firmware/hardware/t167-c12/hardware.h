

#ifndef DEVICE_HARDWARE
#define DEVICE_HARDWARE 1

#ifndef PCINT_vect_num
#  define PCINT1_vect_num	4
#endif


#define USB_CFG_IOPORTNAME      B
#define USB_CFG_DMINUS_BIT      6
#define USB_CFG_DPLUS_BIT       3

/* use pin-change interrupt on D+ */
#define USB_INTR_CFG            PCMSK1
#define USB_INTR_CFG_SET        _BV(PCINT11)
#define USB_INTR_CFG_CLR        0

#define USB_INTR_ENABLE         PCICR
#define USB_INTR_ENABLE_BIT     PCIE1

#define USB_INTR_PENDING        PCIFR
#define USB_INTR_PENDING_BIT    PCIF1
#define USB_INTR_VECTOR         PCINT1_vect
#define USB_INTR_VECTOR_NUM     PCINT1_vect_num


#define WITH_CRYSTAL	1

/*
 * On this device, one rather would like to use PA2,
 * which is DO on th ISP.
 */
#define LED_PORT	B
#define	LED_PIN		1

#endif
