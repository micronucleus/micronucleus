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

#define GNU_SOURCE

#include <stdio.h>
#include <memory.h>
#include <usb.h>
#include <errno.h>

#include <libmnflash/hexdump.h>
#include <libmnflash/usb-device.h>
#include <libmnflash/log.h>

static mnflash_usb_t * mnflash_usb_new() {
	mnflash_usb_t * result = NULL;

	if ( ! (result = malloc(sizeof(mnflash_usb_t))) ) {
		mnflash_error( "cannot allocate memory\n");
		return NULL;
	}

	memset(result, 0, sizeof(mnflash_usb_t));

	return result;
}

void mnflash_usb_destroy(mnflash_usb_t * dev) {
	if ( dev ) {
		if ( dev->handle ) {
			usb_close( dev->handle );
			dev->handle = NULL;
		}
	}
	free(dev);
}

/*
 * Custom read request with retry.
 */
ssize_t mnflash_usb_custom_read( mnflash_usb_t * dev, uint16_t request, uint16_t index, uint16_t value, void * buffer, size_t buflen )
{
	ssize_t bytesRead = 0;
	int	retry = 3;

	do {
		retry --;
		memset(buffer,0,buflen);

		bytesRead = usb_control_msg(
				dev->handle,
				USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
				request,
				value,
				index,
				buffer,
				buflen,
				50);
		if ( bytesRead < 0 ) {
			if ( errno == EPIPE || errno == ETIMEDOUT || errno == EPROTO || errno == EILSEQ ) {
				/*
				 * kernel doc says, these errors may occour
				 * with bad cables or devices ...
				 */
				if ( retry > 0 )
					usleep(20000);
			} else {
				/* more serious error, do not retry */
				retry = 0;
			}
		}

	} while ( bytesRead < 0 && retry > 0 );

	return bytesRead;
}

/*
 * Custom write request with retry.
 */
ssize_t mnflash_usb_custom_write( mnflash_usb_t * dev, uint16_t request, uint16_t index, uint16_t value, void * buffer, size_t buflen )
{
	ssize_t bytesWritten = 0;
	int	retry = 3;

	do {
		retry --;

		bytesWritten = usb_control_msg(
				dev->handle,
				USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
				request,
				value,
				index,
				buffer,
				buflen,
				50);
		if ( bytesWritten < 0 ) {
			if ( errno == EPIPE || errno == ETIMEDOUT || errno == EPROTO || errno == EILSEQ ) {
				/*
				 * kernel doc says, these errors may occour
				 * with bad cables or devices ...
				 */
				if ( retry > 0 )
					usleep(20000);
			} else {
				/* more serious error, do not retry */
				retry = 0;
			}
		}

	} while ( bytesWritten < 0 && retry > 0 );

	return bytesWritten;
}

/*
 * Custom write request without retry.
 */
ssize_t mnflash_usb_custom_write_once( mnflash_usb_t * dev, uint16_t request, uint16_t index, uint16_t value, void * buffer, size_t buflen )
{
	ssize_t bytesWritten = 0;

	bytesWritten = usb_control_msg(
			dev->handle,
			USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
			request,
			value,
			index,
			buffer,
			buflen,
			50);

	return bytesWritten;
}
static const char * mnflash_usb_mode_string(mnflash_usb_t * dev)
{
	switch( dev->mode ) {
		case DEV_MODE_NONE:
			return "unknown mode";
		case DEV_MODE_NORMAL:
			return "normal operation";
		case DEV_MODE_PROGRAMMING:
			return "bootloader";
	}

	return "invalid";
}

void mnflash_usb_show(mnflash_usb_t * dev)
{
	mnflash_msg( "Device in %s, hardware version %d.%d\n",
			mnflash_usb_mode_string(dev),
			dev->major_version,
			dev->minor_version );
}

mnflash_usb_t * mnflash_usb_connect(mnflash_usb_filter filter, void * arg)
{
	mnflash_usb_t *	result = NULL;
	mnflash_usb_mode_t mode = 0;

	usb_dev_handle * handle = NULL;

	struct usb_bus *bus;
	struct usb_device *dev;

	usb_init();
	usb_find_busses();
	usb_find_devices();

	/*
	 * Search for a device
	 */
	for ( bus = usb_get_busses(); bus; bus = bus->next ) {
		for (dev = bus->devices; dev; dev = dev->next) {

			if ( filter ) {
				if ( (handle = filter(dev, arg)) != NULL ) {
					mode = DEV_MODE_NORMAL;
					goto out;
				}
			}

			if ( dev->descriptor.idVendor == MICRONUCLEUS_VENDOR_ID &&
			     dev->descriptor.idProduct == MICRONUCLEUS_PRODUCT_ID)
			{
				/* device in programming mode */
				if ( !(handle = usb_open(dev)) ) {
					mnflash_error( "cannot open USB device: %s\n", usb_strerror());
					goto errout;
				}

				mode = DEV_MODE_PROGRAMMING;
				goto out;
			}
		}
	}

errout:
	if ( handle )
		usb_close(handle);

	return NULL;

out:
	if ( !( result = mnflash_usb_new() ) )
		goto errout;

	result->handle        = handle;
	result->mode          = mode;
	result->major_version = (dev->descriptor.bcdDevice >> 8) & 0xFF;
	result->minor_version = dev->descriptor.bcdDevice & 0xFF;

	return result;


}


