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

#include <stdio.h>
#include <memory.h>
#include <libmnflash/usb-device.h>
#include <libmnflash/uploader.h>
#include <libmnflash/firmware.h>
#include <libmnflash/hexdump.h>

#define DEV_BUF_LEN	8

static mnflash_device_info_t * mnflash_device_info_new(mnflash_usb_t * dev)
{
	mnflash_device_info_t * result = NULL;

	if ( (result = malloc(sizeof(*result))) == NULL )
		return NULL;

	memset(result,0,sizeof(*result));

	result->dev = dev;

	return result;
}

void mnflash_device_info_destroy(mnflash_device_info_t * info)
{
	free(info);
}

mnflash_device_info_t * mnflash_get_device_info(mnflash_usb_t * dev)
{
	uint8_t buffer[DEV_BUF_LEN];
	ssize_t readlen = 0;
	mnflash_device_info_t * result = NULL;

	if ( dev->mode != DEV_MODE_PROGRAMMING ) {
		fprintf(stderr, "device not in bootloader\n");
		return NULL;
	}

	readlen = mnflash_usb_custom_read(dev, BOOTLOADER_INFO, 0, 0, buffer, DEV_BUF_LEN);

	if ( readlen < 4 ) {
		fprintf(stderr, "cannot read bootloader info from device\n");
		if ( readlen < 0 ) {
			fprintf(stderr, "  %s\n", usb_strerror());
		}
		return NULL;
	}

	if ((result = mnflash_device_info_new(dev)) == NULL) {
		fprintf(stderr, "cannot initialize uploader struct\n");
		return NULL;
	}

	/* micronucleus progmem size is big endian */
	result->progmem_size = buffer[1] + (buffer[0] << 8);
	result->page_size    = buffer[2];
	result->write_sleep  = buffer[3];
	if ( readlen > 4 ) {
		result->device = buffer[4];
		result->wiring = buffer[5];
	} else {
		result->device = 255;
		result->wiring = 255;
	}

	return result;
}

void mnflash_info(mnflash_device_info_t * info)
{
	fprintf(stdout, "           device info: %d/%d (%s)\n",
			info->device, info->wiring, mnflash_firmware_get_target_name(info));
	fprintf(stdout, "available program size: %d bytes\n", info->progmem_size);
	fprintf(stdout, "      device page size: %d bytes\n", info->page_size);
	fprintf(stdout, "           write delay: %d ms\n",    info->write_sleep);
}

static int mnflash_erase(mnflash_device_info_t * info)
{
	fprintf(stdout,"erasing device ...\n");
	usleep( 2000 * info->write_sleep );
	if (mnflash_usb_custom_write_once(info->dev, BOOTLOADER_ERASE, 0, 0, NULL, 0) < 0) {
		fprintf(stderr,"cannot erase device%s\n", usb_strerror());
		return 0;
	}

	fprintf(stderr, "sleeping for %dµs\n", (info->write_sleep * ((info->progmem_size/info->page_size) + 1) * 1000));
	usleep(info->write_sleep * ((info->progmem_size/info->page_size) + 1) * 1000);

	return 1;
}

static int mnflash_execute(mnflash_device_info_t * info)
{
	ssize_t written = 0;

	fprintf(stdout,"run application ...\n");
	usleep( 2000 * info->write_sleep );
	if ((written = mnflash_usb_custom_write_once(info->dev, BOOTLOADER_EXECUTE, 0, 0, NULL, 0)) < 0) {
		fprintf(stderr,"cannot execute application: %zd\n", written);
		return 0;
	}
	return 1;
}

int mnflash_upload(mnflash_device_info_t * info, mnflash_firmware_t  * firmware)
{
	uint16_t page_now = 0;
	uint8_t	* page_buffer;

	/*
	 * We cannot read-modify-write pages on the device because the
	 * bootloader does not let us read a page. So for now, the firmware
	 * must start on a page boundry.
	 *
	 * Not much of a problem since usually the firmware starts at 0
	 */
	if ( (firmware->start % info->page_size) != 0 ) {
		fprintf(stderr, "Firmware does not start at a page boundry (page size is %d)\n", info->page_size);
		return 0;
	}

	if ( firmware->end >= info->progmem_size ) {
		fprintf(stderr, "Firmware image is too big\n");
		return 0;
	}

	if ( (page_buffer = malloc(info->page_size)) == NULL ) {
		fprintf(stderr, "Cannot allocate page buffer\n");
		return 0;
	}

	if ( ! mnflash_erase(info) ) {
		fprintf(stderr, "Failed to erase device flash memory\n");
		return 0;
	}

	for (page_now = firmware->start; page_now < firmware->end; page_now += info->page_size) {
		size_t page_bytes = 0;
		ssize_t writelen = 0;

		if (page_now + info->page_size < firmware->end) {
			page_bytes = info->page_size;
		} else {
			page_bytes = firmware->end - page_now;
		}

		memset(page_buffer, 0xFF, info->page_size);
		memcpy(page_buffer, firmware->data + page_now, page_bytes);

		mnflash_hexdump(page_now, page_buffer, page_bytes);

		usleep( 1000 * info->write_sleep );

		writelen = mnflash_usb_custom_write(
				info->dev,
				BOOTLOADER_WRITE_PAGE,
				page_now,
				info->page_size,
				page_buffer,
				info->page_size);


		if ( writelen != info->page_size ) {
			/*
			 * Our last write was not successfull, so what now ?
			 */
			fprintf(stderr, "bad write: addr=0x%x, wanted to write %zd bytes but wrote %zd\n",
					page_now, page_bytes, writelen);

			if ( writelen < 0 ) {
				fprintf(stderr, "  usb error: %s\n", usb_strerror() );
			}
		}
	}

	if ( (page_now / info->page_size) < (info->progmem_size / info->page_size) ) {

		ssize_t writelen = 0;

		page_now = (info->progmem_size / info->page_size) * info->page_size;
		fprintf(stdout,"writing extra page at 0x%x\n", page_now);

		memset(page_buffer, 0xFF, info->page_size);

		usleep( 2000 * info->write_sleep );

		writelen = mnflash_usb_custom_write(
				info->dev,
				BOOTLOADER_WRITE_PAGE,
				page_now,
				info->page_size,
				page_buffer,
				info->page_size);

		if ( writelen != info->page_size ) {
			fprintf(stderr, "could not write tiny table page, expected %d but wrote %zd\n",
					info->page_size, writelen);
			if ( writelen < 0 ) {
				fprintf(stderr, "  usb error: %s\n", usb_strerror() );
			}
			return 0;
		}
	}

	mnflash_execute(info);

	return 1;
}


