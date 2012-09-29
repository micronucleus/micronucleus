/*
	Created: September 2012
	by ihsan Kehribar <ihsan@kehribar.me>
	
	Permission is hereby granted, free of charge, to any person obtaining a copy of
	this software and associated documentation files (the "Software"), to deal in
	the Software without restriction, including without limitation the rights to
	use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
	of the Software, and to permit persons to whom the Software is furnished to do
	so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.	
*/

/***************************************************************/
/* See the micronucleus_lib.h for the function descriptions/comments */
/***************************************************************/
#include "micronucleus_lib.h"
#include "littleWire_util.h"

micronucleus* micronucleus_connect()
{
	micronucleus *tempHandle = NULL;

	usb_init();
	usbOpenDevice(&tempHandle, VENDOR_ID, "*", PRODUCT_ID, "*", "*", NULL, NULL );

	return tempHandle;
}

int micronucleus_getDeviceInfo(micronucleus* deviceHandle, unsigned int* availableMemory, unsigned char* deviceSize, unsigned char* sleepAmount)
{
	int res;
	res =	usb_control_msg(deviceHandle, 0xC0, 0, 0, 0, rxBuffer, 4, USB_TIMEOUT);
	
	if(res!=4)	
		return -1;
	
	*availableMemory = (rxBuffer[0]<<8) + rxBuffer[1];
	*deviceSize = rxBuffer[2];
	*sleepAmount = rxBuffer[3];
	
	return 0;		
}

int micronucleus_eraseFlash(micronucleus* deviceHandle, unsigned int sleepAmount)
{
	int res;
	res = usb_control_msg(deviceHandle, 0xC0, 2, 0, 0, rxBuffer, 0, USB_TIMEOUT);
	delay(sleepAmount);
	if(res!=0)	
		return -1;
	else 
		return 0;
}

int micronucleus_writeFlash(micronucleus* deviceHandle, unsigned int startAddress, unsigned int endAddress, unsigned char* buffer, unsigned char sleepAmount)
{
	unsigned char		tempBuffer[64];
	unsigned int		i;
	unsigned int		k;	
	unsigned int		res;	

	for(i=startAddress;i<(endAddress);i+=64)
	{
		for(k=0;k<64;k++)
			tempBuffer[k]=buffer[i+k];
		
			 res = usb_control_msg(deviceHandle,
					 USB_ENDPOINT_OUT| USB_TYPE_VENDOR | USB_RECIP_DEVICE,
					 1,
					 64,i,
					 tempBuffer, 64,
					 USB_TIMEOUT);
		delay(sleepAmount);
		if(res!=64)	return -1;
	}
	
	return 0;
}

int micronucleus_startApp(micronucleus* deviceHandle)
{
	int res;
	res = usb_control_msg(deviceHandle, 0xC0, 4, 0, 0, rxBuffer, 0, USB_TIMEOUT);
	if(res!=0)	
		return -1;
	else 
		return 0;
}



