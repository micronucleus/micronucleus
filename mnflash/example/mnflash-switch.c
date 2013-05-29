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

#include <usb.h>


/*
 * vUSB shared USB IDs for HID dvices plus vendor and device name.
 */
#define VUSB_VENDOR_ID  0x16c0
#define VUSB_PRODUCT_ID 0x05df

#define USB_VENDOR_NAME	"collax.com"
#define USB_DEVICE_NAME	"iPDU"

usb_dev_handle * my_filter(struct usb_device * dev, void * arg)
{

	char string[128];
	ssize_t	stringlen = 0;
	usb_dev_handle * handle = NULL;


	if ( dev->descriptor.idVendor != VUSB_VENDOR_ID &&
	     dev->descriptor.idProduct != VUSB_PRODUCT_ID )
	{
		return NULL;
	}

	/* maybe device in normal mode ? Look deeper */
	memset(string,0,sizeof(string));

	if ( !(handle = usb_open(dev)) ) {
		fprintf(stderr, "cannot open USB device: %s\n", usb_strerror());
		return NULL;
	}

	if ( !(dev->descriptor.iManufacturer))
		goto errout;

	if ( !(dev->descriptor.iProduct))
		goto errout;

	if ( (stringlen = usb_get_string_simple(
			      handle,
			      dev->descriptor.iManufacturer,
			      string, sizeof(string)
	      )) < 0)
	{
		fprintf(stderr, "cannot retrive manufacturer: %s\n", usb_strerror());
		goto errout;
	}

	if ( strncmp(string,USB_VENDOR_NAME, stringlen) != 0 )
		goto errout;

	if ( (stringlen = usb_get_string_simple(
			      handle,
			      dev->descriptor.iProduct,
			      string, sizeof(string)
	      )) < 0)
	{
		fprintf(stderr, "cannot retrive product: %s\n", usb_strerror());
		goto errout;
	}

	if ( strncmp(string,USB_DEVICE_NAME, stringlen) != 0 )
		goto errout;

	return handle;

errout:
	if (handle)
		usb_close(handle);
	return NULL;
}


void device_enter_bootloader(mnflash_usb_t * dev)
{
	if ( dev->mode == DEV_MODE_PROGRAMMING )
		return;

	/*
	 * send custom command (for this device: 7) to the device, but do not
	 * check the result: The device may not go into usbPoll() again, so we
	 * will not get an ACK.
	 */
	mnflash_usb_custom_write_once(dev, 7, 0, 0, NULL, 0);
}


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
				printf("?? getopt returned character code 0%o ??\n", c);
		}
	}

	fprintf(stdout, "Searching for USB device ...\n");

	retry = 50;
	do {
		if ((dev = mnflash_usb_connect(my_filter, NULL)) == NULL) {
			usleep(100000);
		}
		retry --;
	} while ( retry > 0 && dev == NULL );

	if ( ! dev ) {
		fprintf(stderr, "No devices found\n");
		exit(1);
	}

	if ( dev->mode != DEV_MODE_PROGRAMMING ) {
		// send device into the bootloader and wait for it to show up again

		fprintf(stdout, "Switch device to bootloader ...\n");

		device_enter_bootloader(dev);

		mnflash_usb_destroy(dev); dev = NULL;

		retry = 50;
		do {
			if ((dev = mnflash_usb_connect(NULL, NULL)) == NULL) {
				usleep(100000);
			}
			retry --;
		} while ( retry > 0 && dev == NULL );

		if ( ! dev ) {
			fprintf(stderr, "Device did not enter the bootloader\n");
			exit(1);
		}
	}

	mnflash_usb_show(dev);

	if ((linfo = mnflash_get_device_info(dev)) == NULL) {
		exit(1);
	}
	mnflash_info(linfo);

	if ( firmware_file ) {
		firmware = mnflash_firmware_locate(firmware_file,application,linfo);
		/*
		if (firmware)
			mnflash_firmware_dump(firmware);
		*/
	} else {
		/* auto-locate file */
	}


	if (firmware)
		mnflash_upload(linfo, firmware);

	mnflash_device_info_destroy(linfo);
	mnflash_usb_destroy(dev);
	dev = NULL;

	exit(0);
}

