#ifndef MICRONUCLEUS_LIB_H
#define MICRONUCLEUS_LIB_H

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

/********************************************************************************
* Header files
********************************************************************************/
#if defined WIN
  #include <lusb0_usb.h>    // this is libusb, see http://libusb.sourceforge.net/
#else
  #include <usb.h>        // this is libusb, see http://libusb.sourceforge.net/
#endif
//#include "opendevice.h"      // common code moved to separate module
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
/*******************************************************************************/

/********************************************************************************
* USB details
********************************************************************************/
#define MICRONUCLEUS_VENDOR_ID   0x16D0
#define MICRONUCLEUS_PRODUCT_ID  0x0753
#define MICRONUCLEUS_USB_TIMEOUT 0xFFFF
#define MICRONUCLEUS_MAX_MAJOR_VERSION 1
/*******************************************************************************/

/********************************************************************************
* Declearations
********************************************************************************/
//typedef usb_dev_handle micronucleus;
// representing version number of micronucleus device
typedef struct _micronucleus_version {
  unsigned char major;
  unsigned char minor;
} micronucleus_version;

// handle representing one micronucleus device
typedef struct _micronucleus {
  usb_dev_handle *device;
  // general information about device
  micronucleus_version version;
  unsigned int flash_size;  // programmable size (in bytes) of progmem
  unsigned int page_size;   // size (in bytes) of page
  unsigned int pages;       // total number of pages to program
  unsigned int write_sleep; // milliseconds
  unsigned int erase_sleep; // milliseconds
} micronucleus;

typedef void (*micronucleus_callback)(float progress);

/*******************************************************************************/

/********************************************************************************
* Try to connect to the device
*     Returns: device handle for success, NULL for fail
********************************************************************************/
micronucleus* micronucleus_connect();
/*******************************************************************************/

/********************************************************************************
* Erase the flash memory
********************************************************************************/
int micronucleus_eraseFlash(micronucleus* deviceHandle, micronucleus_callback progress);
/*******************************************************************************/

/********************************************************************************
* Write the flash memory
********************************************************************************/
int micronucleus_writeFlash(micronucleus* deviceHandle, unsigned int program_length,
                            unsigned char* program, micronucleus_callback progress);
/*******************************************************************************/

/********************************************************************************
* Starts the user application
********************************************************************************/
int micronucleus_startApp(micronucleus* deviceHandle);
/*******************************************************************************/

#endif
