
#ifndef LIBMNFLASH_UPLOADER_H
#define LIBMNFLASH_UPLOADER_H 1

#include <libmnflash/usb-device.h>
#include <libmnflash/firmware.h>

enum {
	BOOTLOADER_INFO		= 0,
	BOOTLOADER_WRITE_PAGE	= 1,
	BOOTLOADER_ERASE	= 2,
	BOOTLOADER_EXECUTE	= 3,
} mnflash_bootloader_requests;

typedef struct mnflash_device_info_s {
	mnflash_usb_t *	dev;
	uint16_t	progmem_size;
	uint8_t		page_size;
	uint8_t		write_sleep;
	uint8_t		device;
	uint8_t		wiring;
} mnflash_device_info_t;

extern mnflash_device_info_t * mnflash_get_device_info(mnflash_usb_t * dev);
extern void mnflash_device_info_destroy(mnflash_device_info_t * info);
extern void mnflash_info(mnflash_device_info_t * info);
extern int  mnflash_upload(mnflash_device_info_t * info, mnflash_firmware_t * firmware);


#endif
