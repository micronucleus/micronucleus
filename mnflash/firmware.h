

#ifndef FIRMWARE_H
#define FIRMWARE_H 1

#include "uploader.h"

extern const char * firmware_get_target_name(struct uploader_device_info * linfo);
extern struct firmware_blob * firmware_load_file_autofmt(const char * path);

extern struct firmware_blob * load_firmware_from_dir(
		const char * path,
		const char * app,
		struct uploader_device_info * linfo);

extern struct firmware_blob * firmware_locate(const char * path, const char * app, struct uploader_device_info * linfo);

#endif
