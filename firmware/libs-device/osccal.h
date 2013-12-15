/* Name: osccal.h
 * Author: Christian Starkjohann
 * Creation Date: 2008-04-10
 * Changes 2013-11-04 cpldcpu@gmail.com
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 */

/*
General Description:
This module contains a function which calibrates the AVR's internal RC
oscillator so that the CPU runs at F_CPU (F_CPU is a macro which must be
defined when the module is compiled, best passed in the compiler command
line). The time reference is the USB frame clock of 1 kHz available
immediately after a USB RESET condition. Timing is done by counting CPU
cycles, so all interrupts must be disabled while the calibration runs. 
The size optimized assembler implementation includes its own implementation
of usbMeasureFrameLength. Therefore USB_CFG_HAVE_MEASURE_FRAME_LENGTH should 
be set to 0 to avoid including unused code sections. It is recommended to call
calibrateOscillatorASM() from the reset hook in usbconfig.h by including osccal.h:

#include "osccal.h"

This routine is an alternative to the continuous synchronization described
in osctune.h.

Algorithm used:  See osccalASM.x

Limitations:
This calibration algorithm may try OSCCAL values of up to 192 even if the
optimum value is far below 192. It may therefore exceed the allowed clock
frequency of the CPU in low voltage designs!
Precision depends on the OSCCAL vs. frequency dependency of the oscillator.
Typical precision for an ATMega168 (derived from the OSCCAL vs. F_RC diagram
in the data sheet) should be in the range of 0.4%. Only the 12.8 MHz and
16.5 MHz versions of V-USB (with built-in receiver PLL) can tolerate this
deviation! All other frequency modules require at least 0.2% precision.
*/

#ifndef __OSCCAL_H_INCLUDED__
#define __OSCCAL_H_INCLUDED__

#ifndef __ASSEMBLER__
	void calibrateOscillatorASM(void);
#	define USB_RESET_HOOK(resetStarts)  if(!resetStarts){ calibrateOscillatorASM();}
#	define USB_CFG_HAVE_MEASURE_FRAME_LENGTH   0
#endif	
/* This function calibrates the RC oscillator so that the CPU runs at F_CPU.
 * It MUST be called immediately after the end of a USB RESET condition!
 * Disable all interrupts during the call!
 * It is recommended that you store the resulting value in EEPROM so that a
 * good guess value is available after the next reset.
 */


#endif /* __OSCCAL_H_INCLUDED__ */
