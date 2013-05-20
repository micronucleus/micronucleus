

#ifndef UPLOADER_H
#define UPLOADER_H 1

#include "usb-device.h"
#include "firmware-blob.h"

enum {
	BOOTLOADER_INFO		= 0,
	BOOTLOADER_WRITE_PAGE	= 1,
	BOOTLOADER_ERASE	= 2,
	BOOTLOADER_EXECUTE	= 3,
} bootloader_requests;

struct uploader_device_info {
	device_t *	dev;
	uint16_t	progmem_size;
	uint8_t		page_size;
	uint8_t		write_sleep;
	uint8_t		device;
	uint8_t		wiring;
};

extern struct uploader_device_info * uploader_get_device_info(device_t * dev);
extern void uploader_device_info_destroy(struct uploader_device_info * info);
extern void uploader_info(struct uploader_device_info * info);
extern int uploader_upload(struct uploader_device_info * info, struct firmware_blob * firmware);


#endif
