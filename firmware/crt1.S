
/* Copyright (c) 2002, Marek Michalkiewicz <marekm@amelek.gda.pl>
   Copyright (c) 2007, 2008 Eric B. Weddington
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.

   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE. */

/* $Id$ */

#if (__GNUC__ < 3) || (__GNUC__ == 3 && __GNUC_MINOR__ < 3)
#error "GCC version >= 3.3 required"
#endif

/*
#include IOSYMFILE
#include "macros.inc"
*/
#include <avr/io.h>
//#include <avr/pgmspace.h>

#ifdef __AVR_HAVE_JMP_CALL__
  #define XJMP jmp
#else
  #define XJMP rjmp
#endif  

  #define XCALL rcall
  
	.macro	vector name
	.weak	\name
	.set	\name, __init
	XJMP	\name
	.endm

	.section .vectors,"ax",@progbits
	.global	__vectors
	.global	__bad_interrupt
	.func	__vectors

__bad_interrupt:	
__vectors:
	XJMP	__init
;	vector	__vector_1
;	vector	__vector_2
;	vector	__vector_3
	.endfunc

	/* Handle unexpected interrupts (enabled and no handler), which
	   usually indicate a bug.  Jump to the __vector_default function
	   if defined by the user, otherwise jump to the reset address.

	   This must be in a different section, otherwise the assembler
	   will resolve "rjmp" offsets and there will be no relocs.  */

	.section .init0,"ax",@progbits
	.weak	__init
;	.func	__init
__init:

	.weak	__stack
	.set	__stack, RAMEND
	/* By default, malloc() uses the current value of the stack pointer
	   minus __malloc_margin as the highest available address.

	   In some applications with external SRAM, the stack can be below
	   the data section (in the internal SRAM - faster), and __heap_end
	   should be set to the highest address available for malloc().  */
	.weak	__heap_end
	.set	__heap_end, 0

	.section .init2,"ax",@progbits
	clr		R1
    
#ifndef ENABLE_UNSAFE_OPTIMIZATIONS
	out		0x3f,r1
	ldi		r28,lo8(__stack)
	ldi		r29,hi8(__stack)
	out		0x3d, r28
	out		0x3e, r29
#endif    
	.section .init9,"ax",@progbits
;  rcall main
;  XJMP  __vectors-2
	XJMP	main
  
;	.endfunc

