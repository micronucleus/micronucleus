/*
 *   Created: May 2013
 *   by Andreas Hofmeister <andi@collax.com>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy of
 *   this software and associated documentation files (the "Software"), to deal in
 *   the Software without restriction, including without limitation the rights to
 *   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *   of the Software, and to permit persons to whom the Software is furnished to do
 *   so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <stdio.h>
#include <string.h>
#include <getopt.h>

#include "usb-device.h"
#include "uploader.h"

#include "load-elf.h"
#include "load-ihex.h"
#include "load-raw.h"

struct target_name {
	uint8_t	device;
	uint8_t	wiring;
	char *	target;
};

struct target_name target_list[] = {
	{ .device =  4, .wiring = 1, .target = "tiny-85-1" },
	{ .device =  4, .wiring = 2, .target = "tiny-85-2" },
	{ .device =  7, .wiring = 1, .target = "tiny-861-c12" },
	{ .device = 14, .wiring = 1, .target = "tiny-167-c12" },
	{ .target = NULL},
};

const char * firmware_get_target_name(struct uploader_device_info * linfo) {
	struct target_name * now = NULL;

	for( now = target_list; now->target != NULL; now ++ ) {
		if ( now->device == linfo->device && now->wiring == linfo->wiring )
			return now->target;
	}

	return "unknown";
}

char * extensions[] = {
	".elf",
	".hex",
	".bin",
	".raw",
	NULL
};

char * prefixes[] = {
	"",
	"target-",
	NULL
};

/*
 * Load firmware file with auto-format detection
 */
struct firmware_blob * firmware_load_file_autofmt(const char * path)
{
	struct firmware_blob * firmware = NULL;
	if ( (firmware = elf_load(path)) == NULL) {
		if ( (firmware = ihex_load(path)) == NULL) {
			if ( (firmware = raw_load(path)) == NULL) {
				fprintf(stderr, "cannot load firmware.\n");
				return NULL;
			}
		}
	}

	return firmware;
}

/*
 * Load firmware from directory.
 */
struct firmware_blob * load_firmware_from_dir(const char * path, const char * app, struct uploader_device_info * linfo)
{
	struct stat statbuf;
	struct firmware_blob * firmware = NULL;
	const char * stem = firmware_get_target_name(linfo);
	char * name = NULL;
	size_t len, pflen, extlen;
	int i,j = 0;

	len = pflen = extlen = 0;

	for (i = 0; prefixes[i] != NULL; i ++) {
		if ( pflen < strlen(prefixes[i]) ) {
			pflen = strlen(prefixes[i]);
		}
	}

	for (i = 0; extensions[i] != NULL; i ++) {
		if ( extlen < strlen(extensions[i]) ) {
			extlen = strlen(extensions[i]);
		}
	}

	len = strlen(path) + strlen(stem) + pflen + extlen + 3;

	if (app) {
		len += strlen(app);
	}

	if ((name = malloc(len + 3)) == NULL)
		return NULL;

	for (j = 0; prefixes[j] != NULL; j ++) {
		for (i = 0; extensions[i] != NULL; i ++) {
			if ( app ) {
				snprintf(name, len, "%s/%s%s/%s%s", path, prefixes[j], stem, app, extensions[i]);
			} else {
				snprintf(name, len, "%s/%s%s%s", path, prefixes[j], stem, extensions[i]);
			}
			fprintf(stderr,"trying %s ...\n", name);
			if (stat(name, &statbuf) == 0) {

				if ((statbuf.st_mode & S_IFMT) == S_IFREG) {
					firmware = firmware_load_file_autofmt(name);
				}
				if (firmware)
					break;
			}
		}
		if (firmware)
			break;
	}

	free(name);

	return firmware;
}

struct firmware_blob * firmware_locate(const char * path, const char * app, struct uploader_device_info * linfo)
{
	struct stat statbuf;

	if (stat(path, &statbuf) == 0) {
		if ((statbuf.st_mode & S_IFMT) == S_IFREG) {
			return firmware_load_file_autofmt(path);
		}
		else if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
			return load_firmware_from_dir(path, app, linfo);
		}
	} else {
		/* look for file in search path matching stem and hardware */
	}

	return NULL;
}

