/* Name: usbcalls.h
 * Project: usbcalls library
 * Author: Christian Starkjohann
 * Creation Date: 2006-02-02
 * Tabsize: 4
 * Copyright: (c) 2006 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: Proprietary, free under certain conditions. See Documentation.
 * This Revision: $Id: usbcalls.h 281 2007-03-20 13:22:10Z cs $
 */

#ifndef __usbcalls_h_INCLUDED__
#define __usbcalls_h_INCLUDED__

/*
General Description:
This module implements an abstraction layer for access to USB/HID communication
functions. An implementation based on libusb (portable to Linux, FreeBSD and
Mac OS X) and a native implementation for Windows are provided.
*/

/* ------------------------------------------------------------------------ */

#define USB_HID_REPORT_TYPE_INPUT   1
#define USB_HID_REPORT_TYPE_OUTPUT  2
#define USB_HID_REPORT_TYPE_FEATURE 3
/* Numeric constants for 'reportType' parameters */

#define USB_ERROR_NONE      0
#define USB_ERROR_ACCESS    1
#define USB_ERROR_NOTFOUND  2
#define USB_ERROR_BUSY      16
#define USB_ERROR_IO        5
/* These are the error codes which can be returned by functions of this
 * module.
 */

/* ------------------------------------------------------------------------ */

typedef struct usbDevice    usbDevice_t;
/* This type represents a USB device internally. Only opaque pointers to this
 * type are available outside the module implementation.
 */

/* ------------------------------------------------------------------------ */

int usbOpenDevice(usbDevice_t **device, int vendor, char *vendorName, int product, char *productName, int usesReportIDs);
/* This function opens a USB device. 'vendor' and 'product' are the numeric
 * Vendor-ID and Product-ID of the device we want to open. If 'vendorName' and
 * 'productName' are both not NULL, only devices with matching manufacturer-
 * and product name strings are accepted. If the device uses report IDs,
 * 'usesReportIDs' must be set to a non-zero value.
 * Returns: If a matching device has been found, USB_ERROR_NONE is returned and
 * '*device' is set to an opaque pointer representing the device. The device
 * must be closed with usbCloseDevice(). If the device has not been found or
 * opening failed, an error code is returned.
 */
void    usbCloseDevice(usbDevice_t *device);
/* Every device opened with usbOpenDevice() must be closed with this function.
 */
int usbSetReport(usbDevice_t *device, int reportType, char *buffer, int len);
/* This function sends a report to the device. 'reportType' specifies the type
 * of report (see USB_HID_REPORT_TYPE* constants). The report ID must be in the
 * first byte of buffer and the length 'len' of the report is specified
 * including this report ID. If no report IDs are used, buffer[0] must be set
 * to 0 (dummy report ID).
 * Returns: 0 on success, an error code otherwise.
 */
int usbGetReport(usbDevice_t *device, int reportType, int reportID, char *buffer, int *len);
/* This function obtains a report from the device. 'reportType' specifies the
 * type of report (see USB_HID_REPORT_TYPE* constants). The requested report ID
 * is passed in 'reportID'. The caller must pass a buffer of the size of the
 * expected report in 'buffer' and initialize the variable in '*len' to the
 * total size of this buffer. Upon successful return, the report (prefixed with
 * a report ID) is in 'buffer' and the actual length of the report is returned
 * in '*len'.
 * Returns: 0 on success, an error code otherwise.
 */

/* ------------------------------------------------------------------------ */

#endif /* __usbcalls_h_INCLUDED__ */
