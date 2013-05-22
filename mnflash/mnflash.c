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
#include "firmware.h"
#include "uploader.h"

int main(int argc, char ** argv)
{
	device_t * dev = NULL;
	struct uploader_device_info * linfo	= NULL;
	struct firmware_blob * firmware		= NULL;

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
				printf("?? getopt returned character code 0%o ??\n", c);
		}
	}

	retry = 50;
	do {
		if ((dev = device_connect(1)) == NULL) {
			usleep(100000);
		}
		retry --;
	} while ( retry > 0 && dev == NULL );

	if ( ! dev ) {
		fprintf(stderr, "device did not enter bootloader\n");
		exit(1);
	}

	device_show(dev);

	if ((linfo = uploader_get_device_info(dev)) == NULL) {
		exit(1);
	}
	uploader_info(linfo);

	if ( firmware_file ) {
		firmware = firmware_locate(firmware_file,application,linfo);
//		if (firmware)
//			firmware_dump(firmware);
	} else {
		/* auto-locate file */
	}


	if (firmware)
		uploader_upload(linfo, firmware);


	uploader_device_info_destroy(linfo);
	device_destroy(dev);
	dev = NULL;

	exit(0);
}

