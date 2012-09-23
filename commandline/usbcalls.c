/* Name: usbcalls.c
 * Project: usbcalls library
 * Author: Christian Starkjohann
 * Creation Date: 2006-02-02
 * Tabsize: 4
 * Copyright: (c) 2006 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: Proprietary, free under certain conditions. See Documentation.
 * This Revision: $Id: usbcalls.c 281 2007-03-20 13:22:10Z cs $
 */

/* This file includes the appropriate implementation based on platform
 * specific defines.
 */

#if defined(WIN32)
#   include "usb-windows.c"
#else
/* e.g. defined(__APPLE__) */
#   include "usb-libusb.c"
#endif
