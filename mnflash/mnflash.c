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

#include <libmnflash/usb-device.h>
#include <libmnflash/firmware.h>
#include <libmnflash/uploader.h>
#include <libmnflash/log.h>

int main(int argc, char ** argv)
{
	mnflash_usb_t * dev = NULL;
	mnflash_device_info_t * linfo	= NULL;
	mnflash_firmware_t * firmware	= NULL;

	char *firmware_file	= NULL;
	char * application	= NULL;
	int retry = 50;

	while (1) {
		int option_index = 0;
		int c;

		static const struct option long_options[] = {
			{"application",	required_argument,	0,  0 },
			{"firmware",	required_argument,	0,  0 },
			{"info",	no_argument,		0,  0 },
			{0,		0,			0,  0 }
		};

		c = getopt_long(argc, argv, "", long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case 0:
				if ( strcmp(long_options[option_index].name, "firmware") == 0) {
					firmware_file = strdup(optarg);
				}
				else if ( strcmp(long_options[option_index].name, "application") == 0) {
					application = strdup(optarg);
				}
				break;

			default:
				mnflash_error("?? getopt returned character code 0%o ??", c);
		}
	}

	retry = 50;
	mnflash_msg("looking for a device to flash ...");
	do {
		if ((dev = mnflash_usb_connect(NULL, NULL)) == NULL) {
			usleep(100000);
		}
		retry --;
	} while ( retry > 0 && dev == NULL );

	if ( ! dev ) {
		mnflash_error("device did not enter bootloader");
		exit(1);
	}

	mnflash_usb_show(dev);

	if ((linfo = mnflash_get_device_info(dev)) == NULL) {
		exit(1);
	}
	mnflash_info(linfo);

	if ( firmware_file ) {
		firmware = mnflash_firmware_locate(firmware_file,application,linfo);
		if (firmware)
			mnflash_firmware_dump(firmware);
	}


	if (firmware)
		mnflash_upload(linfo, firmware);


	mnflash_device_info_destroy(linfo);
	mnflash_usb_destroy(dev);
	dev = NULL;

	exit(0);
}

