; Ralph Doncaster 2020
; optimized OSCCAL tuning from low-speed USB SOF every 1ms

; Ralph Doncaster public domain AVR asm macros github.com/nerdralph

.macro GLABEL name
     .global \name
     \name:
.endm

; in/out require an io register at address < 0x40
.macro LOAD rd, ioreg
    .if \ioreg > 0x3F
    lds \rd, \ioreg
    .else
    in \rd, \ioreg
    .endif
.endm

.macro STORE ioreg, rr
    .if \ioreg > 0x3F
    sts \ioreg, \rr
    .else
    out \ioreg, \rr
    .endif
.endm

#define __SFR_OFFSET 0                  /* start SFRs at 0x00 not 0x20 */
#include "usbdrv/usbdrv.h"

/*
; defines for standalone debugging 
#define OSCCAL 0x31
#define USBIN 0x16
#define USBPLUS 4
#define USBMINUS 3
*/

#define countH r25
#define countL r24
#define scratch r23
#define shift r22

; tuneOsccal should be called after USB reset
; needs to see 5 consecutive EOF/SOF transistions
GLABEL tuneOsccal
reset:
    sbis USBIN, USBMINUS
    rjmp reset                          ; wait for bus reset to end
    ;sts NRDR, r1                        ; debug
    rcall countFrame                    ; ignore 1st count
    rcall countFrame
    rcall tuneOnce
    rcall tuneOnce
    ; fall through to tuneOnce for third time

.equ fKHz, (F_CPU/1000)
.equ goal, (fKHz / 12) * fraction       ; each fraction is 12 cycles
tuneOnce:
    subi countH, hi8(goal)              ; countH-goal = change to OSCCAL 
    LOAD scratch, OSCCAL
    sub scratch, countH
    STORE OSCCAL, scratch
    ;sts NRDR, countH                    ; debug
    ; fall through to countFrame for next tuning

; countFrame counts the time to the next EOF/SOF
; end-of-frame has an idle guard band followed by SE0 (D+ & D- low)
; idle is D+ low & D- high. save D+ history in shift
; countL is used as a fractional accumulator with countH LSbit =~ 0.5%
; fraction = (256/.5)/fMHz rounded
.equ fraction, ((2 * 512)/(fKHz / 1000) + 1) / 2
countFrame:
    ldi shift, 0xFF
    ldi countL, fraction                ; countFrame overhead adjustment
    clr countH
waitSOF:                                ; 12-cycle loop
    lsr shift
    in scratch, USBIN
    or scratch, r0
    adiw countL, fraction
    bst scratch, USBPLUS 
    bld shift, 7
    in r0, USBIN
    sbrs r0, USBMINUS
    tst shift
    brne waitSOF                        ; not idle last 7 loops
ret

