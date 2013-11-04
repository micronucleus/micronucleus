/* Name: osccal.c
 * Author: Christian Starkjohann
 * Creation Date: 2008-04-10
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 * This Revision: $Id: osccal.c 762 2009-08-12 17:10:30Z cs $
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#ifndef uchar
#define uchar   unsigned char
#endif

int usbMeasureFrameLengthDecreasing(int);

/* ------------------------------------------------------------------------- */
/* ------------------------ Oscillator Calibration ------------------------- */
/* ------------------------------------------------------------------------- */

/* Calibrate the RC oscillator. Our timing reference is the Start Of Frame
 * signal (a single SE0 bit) repeating every millisecond immediately after
 * a USB RESET. 
 *
 * 
 * Optimized version by cpldcpu@gmail.com, Nov 3rd 2013.
 *
 * Benefits:
 *	  - Codesize reduced by 54 bytes.
 *    - Improved robustness due to removing timeout from frame length measurement and 
 *	    inserted NOP after OSCCAL writes.
 *
 * Changes:
 *    - The new routine performs a combined binary and neighborhood search
 *      in a single loop.
 *      Note that the neighborhood search is necessary due to the quasi-monotonic 
 *      nature of OSCCAL. (See Atmel application note AVR054).
 *	  - Inserted NOP after writes to OSCCAL to avoid CPU errors during oscillator
 *      stabilization. 
 *    - Implemented new routine to measure frame time "usbMeasureFrameLengthDecreasing".
 *		This routine takes the target time as a parameter and returns the deviation.
 *	  - usbMeasureFrameLengthDecreasing measures in multiples of 5 cycles and is thus
 *	    slighly more accurate.
 *	  - usbMeasureFrameLengthDecreasing does not support time out anymore. The original
 *	    implementation returned zero in case of time out, which would have caused the old
 *      calibrateOscillator() implementation to increase OSSCAL to 255, effictively
 *      overclocking and most likely crashing the CPU. The new implementation will enter
 *		an infinite loop when no USB activity is encountered. The user program should
 *      use the watchdog to escape from situations like this.
  * 
 * This routine will work both on controllers with and without split OSCCAL range.
 * The first trial value is 128 which is the lowest value of the upper OSCCAL range
 * on Attiny85 and will effectively limit the search to the upper range, unless the
 * RC oscillator frequency is unusually high. Under normal operation, the highest 
 * tested frequency setting is 192. This corresponds to ~20 Mhz core frequency and 
 * is still within spec for a 5V device.
 */

void    calibrateOscillator(void)
{
	uchar       step, trialValue, optimumValue;
	int         x, targetValue;
	uchar		optimumDev;
	uchar		i,xl;
	
	targetValue = (unsigned)((double)F_CPU * 999e-6 / 5.0 + 0.5);  /* Time is measured in multiples of 5 cycles. Target is 0.999µs */
    optimumDev = 0xff;   
  //  optimumValue = OSCCAL; 
	step=64;
	trialValue = 128;
	
	cli(); // disable interrupts
	
	/*
		Performs seven iterations of a binary search (stepwidth decreasing9
		with three additional steps of a neighborhood search (step=1, trialvalue will oscillate around target value to find optimum)
	*/

	for(i=0; i<10; i++){
		OSCCAL = trialValue;
		asm volatile(" NOP");
	
		x = usbMeasureFrameLengthDecreasing(targetValue);

		if(x < 0)             /* frequency too high */
		{
			trialValue -= step;
			xl=(uchar)-x;
		}
		else                  /* frequency too low */
		{
			trialValue += step;			
			xl=(uchar)x;
		}
		
		/*
			Halve stepwidth to perform binary search. Logical oring with 1 to ensure step is never equal to zero. 
			This results in a neighborhood search with stepwidth 1 after binary search is finished.			
			Once the neighbourhood search stage is reached, x will be smaller than +-255, hence more code can be
			saved by only working with the lower 8 bits.
		*/
		
		step >>= 1;
		
		if (step==0)
		{
			step=1;			
			if(xl <= optimumDev){
				optimumDev = xl;
				optimumValue = OSCCAL;
			}
		}

	}

	OSCCAL = optimumValue;
	asm volatile(" NOP");
	
	sei(); // enable interrupts
}

void    calibrateOscillator_old(void)
{
	uchar       step, trialValue, optimumValue;
	int         x, optimumDev, targetValue;
	uchar		i;
	
	targetValue = (unsigned)((double)F_CPU * 999e-6 / 5.0 + 0.5);  /* Time is measured in multiples of 5 cycles. Target is 0.999µs */
    optimumDev = 0x7f00;   // set to high positive value
    optimumValue = OSCCAL; 
	step=64;
	trialValue = 128;
	
	/*
		Performs seven iterations of a binary search (stepwidth decreasing9
		with three additional steps of a neighborhood search (step=1, trialvalue will oscillate around target value to find optimum)
	*/

	for(i=0; i<10; i++){
		OSCCAL = trialValue;
		asm volatile(" NOP");
	
		x = usbMeasureFrameLengthDecreasing(targetValue);

		if(x < 0)             /* frequency too high */
		{
			trialValue -= step;
			x = -x;
		}
		else                  /* frequency too low */
		{
			trialValue += step;			
		}
		
		/*
			Halve stepwidth to perform binary search. Logical oring with 1 to ensure step is never equal to zero. 
			This results in a neighborhood search with stepwidth 1 after binary search is finished.			
		*/
		
		step >>= 1;
		step |=1;	

		if(x < optimumDev){
			optimumDev = x;
			optimumValue = OSCCAL;
		}
	}

	OSCCAL = optimumValue;
	asm volatile(" NOP");
}

/*
Note: This calibration algorithm may try OSCCAL values of up to 192 even if
the optimum value is far below 192. It may therefore exceed the allowed clock
frequency of the CPU in low voltage designs!
You may replace this search algorithm with any other algorithm you like if
you have additional constraints such as a maximum CPU clock.
For version 5.x RC oscillators (those with a split range of 2x128 steps, e.g.
ATTiny25, ATTiny45, ATTiny85), it may be useful to search for the optimum in
both regions.
*/
