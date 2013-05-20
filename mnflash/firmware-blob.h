

#ifndef FIRMWARE_BLOB_H
#define FIRMWARE_BLOB_H 1

struct firmware_blob {

	char *    filename;

	uint8_t   signature[3];	/* target device signature */

	size_t    entry;	/* start execution here, as index in 'data' */
	size_t	  start;	/* index of the first initialized byte in 'blob' */
	size_t	  end;		/* index of the first uninitialized byte in 'blob' */

	size_t    data_size;
	uint8_t * data;
};

extern struct firmware_blob * firmware_blob_new(void);
void firmware_blob_resize(struct firmware_blob * blob, size_t address);
void firmware_blob_destroy(struct firmware_blob * blob);

extern void firmware_info(const struct firmware_blob * blob);
extern void firmware_dump(const struct firmware_blob * blob);

#endif
