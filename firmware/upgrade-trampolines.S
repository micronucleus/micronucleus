
#include <avr/io.h>
#include <hardware.h>
#include <chipid.h>

        .macro  tvector name
        .if (. - final_vectors < _VECTORS_SIZE)
	.weak   \name
#if VECTOR_SIZE == 4
	jmp	\name
#else
	rjmp	\name
#endif
        .endif
        .endm

	/*
	 * Note: this table must go into a section .vector or .jumptables !
	 *
	 *  The linker has special rules for jump relaxation, but only for
	 *  said sections. The names are hardcoded into gnu ld.
	 */
        .section .vectors,"ax",@progbits
        .global final_vectors
        .func   final_vectors
final_vectors:
	tvector  app__init
	tvector  app__vector_1
	tvector  app__vector_2
	tvector  app__vector_3
	tvector  app__vector_4
	tvector  app__vector_5
	tvector  app__vector_6
	tvector  app__vector_7
	tvector  app__vector_8
	tvector  app__vector_9
	tvector  app__vector_10
	tvector  app__vector_11
	tvector  app__vector_12
	tvector  app__vector_13
	tvector  app__vector_14
	tvector  app__vector_15
	tvector  app__vector_16
	tvector  app__vector_17
	tvector  app__vector_18
	tvector  app__vector_19
	tvector  app__vector_20
	tvector  app__vector_21
	tvector  app__vector_22
	tvector  app__vector_23
	tvector  app__vector_24
	tvector  app__vector_25
	tvector  app__vector_26
	tvector  app__vector_27
	tvector  app__vector_28
	tvector  app__vector_29
	tvector  app__vector_30
	tvector  app__vector_31
	tvector  app__vector_32
	tvector  app__vector_33
	tvector  app__vector_34
	tvector  app__vector_35
	tvector  app__vector_36
	tvector  app__vector_37
	tvector  app__vector_38
	tvector  app__vector_39
	tvector  app__vector_40
	tvector  app__vector_41
	tvector  app__vector_42
	tvector  app__vector_43
	tvector  app__vector_44
	tvector  app__vector_45
	tvector  app__vector_46
	tvector  app__vector_47
	tvector  app__vector_48
	tvector  app__vector_49
	tvector  app__vector_50
	tvector  app__vector_51
	tvector  app__vector_52
	tvector  app__vector_53
	tvector  app__vector_54
	tvector  app__vector_55
	tvector  app__vector_56
	tvector  app__vector_57
	tvector  app__vector_58
	tvector  app__vector_59
	tvector  app__vector_60
	tvector  app__vector_61
	tvector  app__vector_62
	tvector  app__vector_63
	tvector  app__vector_64
	tvector  app__vector_65
	tvector  app__vector_66
	tvector  app__vector_67
	tvector  app__vector_68
	tvector  app__vector_69
	tvector  app__vector_70
	tvector  app__vector_71
	tvector  app__vector_72
	tvector  app__vector_73
	tvector  app__vector_74
	tvector  app__vector_75
	tvector  app__vector_76
	tvector  app__vector_77
	tvector  app__vector_78
	tvector  app__vector_79
	tvector  app__vector_80
	tvector  app__vector_81
	tvector  app__vector_82
	tvector  app__vector_83
	tvector  app__vector_84
	tvector  app__vector_85
	tvector  app__vector_86
	tvector  app__vector_87
	tvector  app__vector_88
	tvector  app__vector_89
	tvector  app__vector_90
	tvector  app__vector_91
	tvector  app__vector_92
	tvector  app__vector_93
	tvector  app__vector_94
	tvector  app__vector_95
	tvector  app__vector_96
	tvector  app__vector_97
	tvector  app__vector_98
	tvector  app__vector_99
	tvector  app__vector_100
	tvector  app__vector_101
	tvector  app__vector_102
	tvector  app__vector_103
	tvector  app__vector_104
	tvector  app__vector_105
	tvector  app__vector_106
	tvector  app__vector_107
	tvector  app__vector_108
	tvector  app__vector_109
	tvector  app__vector_110
	tvector  app__vector_111
	tvector  app__vector_112
	tvector  app__vector_113
	tvector  app__vector_114
	tvector  app__vector_115
	tvector  app__vector_116
	tvector  app__vector_117
	tvector  app__vector_118
	tvector  app__vector_119
	tvector  app__vector_120
	tvector  app__vector_121
	tvector  app__vector_122
	tvector  app__vector_123
	tvector  app__vector_124
	tvector  app__vector_125
	tvector  app__vector_126
	tvector  app__vector_127
	.endfunc

