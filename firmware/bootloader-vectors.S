
#include <avr/io.h>
#include <hardware.h>
#include <chipid.h>

        .macro  bvector num
        .if (. - initial_vectors < _VECTORS_SIZE)
	.if (\num == USB_INTR_VECTOR_NUM)
#if (VECTOR_SIZE == 4)
	rjmp __wrap_vusb_intr
	nop
	.else
	rjmp __my_reset2
	nop
#else
	rjmp __wrap_vusb_intr
	.else
	rjmp __my_reset2
#endif
	.endif
        .endif
        .endm

        .section .vectorpage,"ax",@progbits
        .global initial_vectors
        .func   initial_vectors
initial_vectors:
	bvector  0
	bvector  1
	bvector  2
	bvector  3
	bvector  4
	bvector  5
	bvector  6
	bvector  7
	bvector  8
	bvector  9
	bvector  10
	bvector  11
	bvector  12
	bvector  13
	bvector  14
	bvector  15
	bvector  16
	bvector  17
	bvector  18
	bvector  19
	bvector  20
	bvector  21
	bvector  22
	bvector  23
	bvector  24
	bvector  25
	bvector  26
	bvector  27
	bvector  28
	bvector  29
	bvector  30
	bvector  31
	bvector  32
	bvector  33
	bvector  34
	bvector  35
	bvector  36
	bvector  37
	bvector  38
	bvector  39
	bvector  40
	bvector  41
	bvector  42
	bvector  43
	bvector  44
	bvector  45
	bvector  46
	bvector  47
	bvector  48
	bvector  49
	bvector  50
	bvector  51
	bvector  52
	bvector  53
	bvector  54
	bvector  55
	bvector  56
	bvector  57
	bvector  58
	bvector  59
	bvector  60
	bvector  61
	bvector  62
	bvector  63
	bvector  64
	bvector  65
	bvector  66
	bvector  67
	bvector  68
	bvector  69
	bvector  70
	bvector  71
	bvector  72
	bvector  73
	bvector  74
	bvector  75
	bvector  76
	bvector  77
	bvector  78
	bvector  79
	bvector  80
	bvector  81
	bvector  82
	bvector  83
	bvector  84
	bvector  85
	bvector  86
	bvector  87
	bvector  88
	bvector  89
	bvector  90
	bvector  91
	bvector  92
	bvector  93
	bvector  94
	bvector  95
	bvector  96
	bvector  97
	bvector  98
	bvector  99
	bvector  100
	bvector  101
	bvector  102
	bvector  103
	bvector  104
	bvector  105
	bvector  106
	bvector  107
	bvector  108
	bvector  109
	bvector  110
	bvector  111
	bvector  112
	bvector  113
	bvector  114
	bvector  115
	bvector  116
	bvector  117
	bvector  118
	bvector  119
	bvector  120
	bvector  121
	bvector  122
	bvector  123
	bvector  124
	bvector  125
	bvector  126
	bvector  127
	.endfunc

