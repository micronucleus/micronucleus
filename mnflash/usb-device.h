

#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>
#include <usb.h>

#define MICRONUCLEUS_VENDOR_ID   0x16d0
#define MICRONUCLEUS_PRODUCT_ID  0x0753

typedef enum {
	DEV_MODE_NONE,
	DEV_MODE_NORMAL,
	DEV_MODE_PROGRAMMING,
} device_mode_t;

typedef struct {
	usb_dev_handle*	handle;
	device_mode_t	mode;
	uint8_t		i2c_addr;
	union {
		uint16_t	intversion;
		uint8_t		minormajor[2];
	} version;
} device_t;

#define minor_version	version.minormajor[0]
#define major_version	version.minormajor[1]

extern void device_destroy(device_t * dev);

extern ssize_t device_custom_read( device_t * dev, uint16_t request, uint16_t index, uint16_t value, void * buffer, size_t buflen );
extern ssize_t device_custom_write( device_t * dev, uint16_t request, uint16_t index, uint16_t value, void * buffer, size_t buflen );

extern void device_show(device_t * dev);
extern device_t * device_connect(int loader_only);

#endif
