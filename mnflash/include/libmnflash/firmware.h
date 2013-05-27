

#ifndef LIBMNFLASH_FIRMWARE_H
#define LIBMNFLASH_FIRMWARE_H 1

typedef struct {

	char *    filename;

	uint8_t   signature[3];	/* target device signature */

	size_t    entry;	/* start execution here, as index in 'data' */
	size_t	  start;	/* index of the first initialized byte in 'blob' */
	size_t	  end;		/* index of the first uninitialized byte in 'blob' */

	size_t    data_size;
	uint8_t * data;
} mnflash_firmware_t;

mnflash_firmware_t * mnflash_firmware_new(void);
void mnflash_firmware_resize(mnflash_firmware_t * blob, size_t address);
void mnflash_firmware_destroy(mnflash_firmware_t * blob);

extern void mnflash_firmware_info(const mnflash_firmware_t * blob);
extern void mnflash_firmware_dump(const mnflash_firmware_t * blob);

extern mnflash_firmware_t * mnflash_firmware_load_file_autofmt(const char * path);


typedef struct mnflash_device_info_s mnflash_device_info_t;

extern const char * mnflash_firmware_get_target_name(mnflash_device_info_t * linfo);

extern mnflash_firmware_t * mnflash_load_firmware_from_dir(
		const char * path,
		const char * app,
		mnflash_device_info_t * linfo);

extern mnflash_firmware_t * mnflash_firmware_locate(
		const char * path,
		const char * app,
		mnflash_device_info_t * linfo);

#endif
