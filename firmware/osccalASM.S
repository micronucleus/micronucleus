/* Name: osccalASM.S   v1.1
 * Author: cpldcpu@gmail.com
 * Creation Date: 2013-11-3
 * Update       : 2014-01-4
 * 
 * Tabsize: 4
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 */

/* Calibrate the RC oscillator. Our timing reference is the Start Of Frame
 * signal (a single SE0 bit) repeating every millisecond immediately after
 * a USB RESET. 
 *
 *
 * Benefits:
 *    - Codesize reduced by 90 bytes.
 *    - Improved robustness due to removing timeout from frame length measurement and 
 *      inserted NOP after OSCCAL writes.
 *
 * Changes:
 *    - The new routine performs a combined binary and neighborhood search
 *      in a single loop.
 *      Note that the neighborhood search is necessary due to the quasi-monotonic 
 *      nature of OSCCAL. (See Atmel application note AVR054).
 *    - Inserted NOP after writes to OSCCAL to avoid CPU errors during oscillator
 *      stabilization. 
 *    - Implemented new routine to measure frame time "usbMeasureFrameLengthDecreasing".
 *      This routine takes the target time as a parameter and returns the deviation.
 *    - usbMeasureFrameLengthDecreasing measures in multiples of 5 cycles and is thus
 *      slighly more accurate.
 *    - usbMeasureFrameLengthDecreasing does not support time out anymore. The original
 *      implementation returned zero in case of time out, which would have caused the old
 *      calibrateOscillator() implementation to increase OSSCAL to 255, effictively
 *      overclocking and most likely crashing the CPU. The new implementation will enter
 *      an infinite loop when no USB activity is encountered. The user program should
 *      use the watchdog to escape from situations like this.
 * Update 2014-01-4
 *    - Added an initial sync-state, which will discard the first delay measurement. 
 *      This allows to call this routine before or during the SE0 bus reset without 
 *      corrupting OSCCAL.
 *     - Removed CLI/SEI to allow more flexibility.
 * 
 * 
 * This routine will work both on controllers with and without split OSCCAL range.
 * The first trial value is 128 which is the lowest value of the upper OSCCAL range
 * on Attiny85 and will effectively limit the search to the upper range, unless the
 * RC oscillator frequency is unusually high. Under normal operation, the highest 
 * tested frequency setting is 192. This corresponds to ~20 Mhz core frequency and 
 * is still within spec for a 5V device.
 */
 

#define __SFR_OFFSET 0      /* used by avr-libc's register definitions */
#include "./usbdrv/usbdrv.h"         /* for common defs */

#ifdef __IAR_SYSTEMS_ASM__
/* Register assignments for usbMeasureFrameLengthDecreasing on IAR cc */
/* Calling conventions on IAR:
 * First parameter passed in r16/r17, second in r18/r19 and so on.
 * Callee must preserve r4-r15, r24-r29 (r28/r29 is frame pointer)
 * Result is passed in r16/r17
 * In case of the "tiny" memory model, pointers are only 8 bit with no
 * padding. We therefore pass argument 1 as "16 bit unsigned".
 */

//Untested

#   define i		r20	
#   define opV		r19
#   define opD		r18
#   define try		r21
#   define stp		r22

#   define cnt16L   r30
#   define cnt16H   r31


#else  /* __IAR_SYSTEMS_ASM__ */ 
/* Register assignments for usbMeasureFrameLength on gcc */
/* Calling conventions on gcc:
 * First parameter passed in r24/r25, second in r22/23 and so on.
 * Callee must preserve r1-r17, r28/r29
 * Result is passed in r24/r25
 */

#   define i		r20	
#   define opV		r19
#   define opD		r18
#   define try		r27
#   define stp		r26
#   define cnt16L   r24
#   define cnt16H   r25
#endif
#   define cnt16    cnt16L

#if (OSCCAL_HAVE_XTAL == 0)

.global calibrateOscillatorASM
calibrateOscillatorASM:

  ldi   opD, 255

  ldi   try, 128    ; calibration start value
  ldi   stp, 0      ; initial step width=0 for sync phase (first delay is discarded)
  ldi   i, 10       ; 10 iterations (1x sync, 7x binary search, 2x neighbourhood)

usbCOloop:

#if OSCCAL <64
  out   OSCCAL, try
#else
  sts   OSCCAL, try
#endif  
  nop

usbCOLoopNoCal:

  ; Delay values = F_CPU * 999e-6 / 5 + 0.5
  
#if (F_CPU == 16500000)
  ldi   cnt16L, lo8(3297)
  ldi   cnt16H, hi8(3297)
#elif (F_CPU == 12800000)
  ldi   cnt16L, lo8(2557)
  ldi   cnt16H, hi8(2557)
#elif (F_CPU == 12000000)
  ldi   cnt16L, lo8(2398)
  ldi   cnt16H, hi8(2398)
#elif (F_CPU == 16000000)
  ldi   cnt16L, lo8(3197)
  ldi   cnt16H, hi8(3197)  
#elif (F_CPU == 15000000)
  ldi   cnt16L, lo8(2998)
  ldi   cnt16H, hi8(2998)  
#else
	#error "calibrateOscillatorASM: no delayvalues defined for this F_CPU setting"
#endif

usbCOWaitStrobe:            ; first wait for D- == 0 (idle strobe)
    sbic    USBIN, USBMINUS ;
    rjmp    usbCOWaitStrobe ;
usbCOWaitIdle:              ; then wait until idle again
    sbis    USBIN, USBMINUS ;1 wait for D- == 1
    rjmp    usbCOWaitIdle   ;2
usbCOWaitLoop:
	sbiw	cnt16,1			;[0] [5]
    sbic    USBIN, USBMINUS ;[2] 
    rjmp    usbCOWaitLoop   ;[3]

/*

    ; This section of code deals with traffic from other USB devices on the same hub on USB1.1 hubs.
    ; If this code is excluded, micronucleus may only work when it is connected to a dedicated USB port
    
    ; Disabled due to issues with some Win10 USB drivers, which shorten the time between reset and
    ; first datapacket to less than 20ms.
   
#ifndef ENABLE_UNSAFE_OPTIMIZATIONS
    sbis    USBIN, USBPLUS  ; ignore frame if data is present
    rjmp    usbCOnotdata

usbCOWaitNoData:
    in      cnt16H, USBIN   ; wait for SE0 state (both lines low)
    andi    cnt16H, (1<<USBPLUS)|(1<<USBMINUS)
    brne    usbCOWaitNoData
    in      cnt16H, USBIN   ; be sure SE0 state wasn't a glitch
    andi    cnt16H, (1<<USBPLUS)|(1<<USBMINUS)
    brne    usbCOWaitNoData
usbCOWaitNoData2:
    sbis    USBIN, USBMINUS ; wait for D- go to high
    rjmp    usbCOWaitNoData2
    rjmp    usbCOLoopNoCal
#endif

*/
    
usbCOnotdata:
	sbrs	cnt16H, 7		;delay overflow?
	rjmp	usbCOclocktoolow
	sub		try, stp
	neg		cnt16L
	rjmp	usbCOclocktoohigh
usbCOclocktoolow:
	add		try, stp
usbCOclocktoohigh:
	lsr		stp                ; stp = 0 in first iteration (sync)
                           ; stp = 2^x (x=0..6) during binary search, 
                           ; stp = 1 during neighbourhood search                          
                           
    brne	usbCObinarysearch
    ldi   stp, 64            ; stp=64 to initiate binary search. 
                           ; If we are in neighbourhood search (c=1), it is changed to 1 below
    brcc  usbCObinarysearch
    cp		opD, cnt16L
    brcs	usbCOnoimprovement
#if OSCCAL <64
    in		opV, OSCCAL
#else
    lds		opV, OSCCAL
#endif      
	mov		opD, cnt16L
usbCOnoimprovement:
	ldi		stp, 1             ; stp=1 to continue with neighbourhood search
usbCObinarysearch:  
	subi	i, 1
	brne	usbCOloop

#if OSCCAL <64
  out   OSCCAL, opV
#else
  sts   OSCCAL, opV
#endif  
  nop
  ret

#endif //OSCCAL_HAVE_XTAL  
  
#undef i
#undef opV
#undef opD
#undef try
#undef stp
#undef cnt16
#undef cnt16L
#undef cnt16H

/* ------------------------------------------------------------------------- */
/* ------ Original C Implementation of improved calibrateOscillator -------- */
/* ----------------------   for Reference only ----------------------------- */
/* ------------------------------------------------------------------------- */

#if 0
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
			Halve stepwidth to perform binary search. At step=1 the mode changes to neighbourhood search.
			Once the neighbourhood search stage is reached, x will be smaller than +-255, hence more code can be
			saved by only working with the lower 8 bits.
		*/
		
		step >>= 1;
		
		if (step==0)   // Enter neighborhood search mode
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
}
#endif