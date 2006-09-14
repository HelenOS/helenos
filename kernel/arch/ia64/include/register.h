/*
 * Copyright (C) 2005 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup ia64	
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_REGISTER_H_
#define KERN_ia64_REGISTER_H_

#define CR_IVR_MASK	0xf
#define PSR_IC_MASK	0x2000
#define PSR_I_MASK	0x4000
#define PSR_PK_MASK	0x8000

#define PSR_DT_MASK	(1<<17)
#define PSR_RT_MASK	(1<<27)

#define PSR_DFL_MASK	(1<<18)
#define PSR_DFH_MASK	(1<<19)

#define PSR_IT_MASK	0x0000001000000000

#define PSR_CPL_SHIFT		32
#define PSR_CPL_MASK_SHIFTED	3

#define PFM_MASK        (~0x3fffffffff)

#define RSC_MODE_MASK	3
#define RSC_PL_MASK	12

/** Application registers. */
#define AR_KR0		0
#define AR_KR1		1
#define AR_KR2		2
#define AR_KR3		3
#define AR_KR4		4
#define AR_KR5		5
#define AR_KR6		6
#define AR_KR7		7
/* AR 8-15 reserved */
#define AR_RSC		16
#define AR_BSP		17
#define AR_BSPSTORE	18
#define AR_RNAT		19
/* AR 20 reserved */
#define AR_FCR		21
/* AR 22-23 reserved */
#define AR_EFLAG	24
#define AR_CSD		25
#define AR_SSD		26
#define AR_CFLG		27
#define AR_FSR		28
#define AR_FIR		29
#define AR_FDR		30
/* AR 31 reserved */
#define AR_CCV		32
/* AR 33-35 reserved */
#define AR_UNAT		36
/* AR 37-39 reserved */
#define AR_FPSR		40
/* AR 41-43 reserved */
#define AR_ITC		44
/* AR 45-47 reserved */
/* AR 48-63 ignored */
#define AR_PFS		64
#define AR_LC		65
#define AR_EC		66
/* AR 67-111 reserved */
/* AR 112-127 ignored */

/** Control registers. */
#define CR_DCR		0
#define CR_ITM		1
#define CR_IVA		2
/* CR3-CR7 reserved */
#define CR_PTA		8
/* CR9-CR15 reserved */
#define CR_IPSR		16
#define CR_ISR		17
/* CR18 reserved */
#define CR_IIP		19
#define CR_IFA		20
#define CR_ITIR		21
#define CR_IIPA		22
#define CR_IFS		23
#define CR_IIM		24
#define CR_IHA		25
/* CR26-CR63 reserved */
#define CR_LID		64
#define CR_IVR		65
#define CR_TPR		66
#define CR_EOI		67
#define CR_IRR0		68
#define CR_IRR1		69
#define CR_IRR2		70
#define CR_IRR3		71
#define CR_ITV		72
#define CR_PMV		73
#define CR_CMCV		74
/* CR75-CR79 reserved */
#define CR_LRR0		80
#define CR_LRR1		81
/* CR82-CR127 reserved */

#ifndef __ASM__

#include <arch/types.h>

/** Processor Status Register. */
union psr {
	uint64_t value;
	struct {
		unsigned : 1;
		unsigned be : 1;	/**< Big-Endian data accesses. */
		unsigned up : 1;	/**< User Performance monitor enable. */
		unsigned ac : 1;	/**< Alignment Check. */
		unsigned mfl : 1;	/**< Lower floating-point register written. */
		unsigned mfh : 1;	/**< Upper floating-point register written. */
		unsigned : 7;
		unsigned ic : 1;	/**< Interruption Collection. */
		unsigned i : 1;		/**< Interrupt Bit. */
		unsigned pk : 1;	/**< Protection Key enable. */
		unsigned : 1;
		unsigned dt : 1;	/**< Data address Translation. */
		unsigned dfl : 1;	/**< Disabled Floating-point Low register set. */
		unsigned dfh : 1;	/**< Disabled Floating-point High register set. */
		unsigned sp : 1;	/**< Secure Performance monitors. */
		unsigned pp : 1;	/**< Privileged Performance monitor enable. */
		unsigned di : 1;	/**< Disable Instruction set transition. */
		unsigned si : 1;	/**< Secure Interval timer. */
		unsigned db : 1;	/**< Debug Breakpoint fault. */
		unsigned lp : 1;	/**< Lower Privilege transfer trap. */
		unsigned tb : 1;	/**< Taken Branch trap. */
		unsigned rt : 1;	/**< Register Stack Translation. */
		unsigned : 4;
		unsigned cpl : 2;	/**< Current Privilege Level. */
		unsigned is : 1;	/**< Instruction Set. */
		unsigned mc : 1;	/**< Machine Check abort mask. */
		unsigned it : 1;	/**< Instruction address Translation. */
		unsigned id : 1;	/**< Instruction Debug fault disable. */
		unsigned da : 1;	/**< Disable Data Access and Dirty-bit faults. */
		unsigned dd : 1;	/**< Data Debug fault disable. */
		unsigned ss : 1;	/**< Single Step enable. */
		unsigned ri : 2;	/**< Restart Instruction. */
		unsigned ed : 1;	/**< Exception Deferral. */
		unsigned bn : 1;	/**< Register Bank. */
		unsigned ia : 1;	/**< Disable Instruction Access-bit faults. */
	} __attribute__ ((packed));
};
typedef union psr psr_t;

/** Register Stack Configuration Register */
union rsc {
	uint64_t value;
	struct {
		unsigned mode : 2;
		unsigned pl : 2;	/**< Privilege Level. */
		unsigned be : 1;	/**< Big-endian. */
		unsigned : 11;
		unsigned loadrs : 14;
	} __attribute__ ((packed));
};
typedef union rsc rsc_t;

/** External Interrupt Vector Register */
union cr_ivr {
	uint8_t  vector;
	uint64_t value;
};

typedef union cr_ivr cr_ivr_t;

/** Task Priority Register */
union cr_tpr {
	struct {
		unsigned : 4;
		unsigned mic: 4;		/**< Mask Interrupt Class. */
		unsigned : 8;
		unsigned mmi: 1;		/**< Mask Maskable Interrupts. */
	} __attribute__ ((packed));
	uint64_t value;
};

typedef union cr_tpr cr_tpr_t;

/** Interval Timer Vector */
union cr_itv {
	struct {
		unsigned vector : 8;
		unsigned : 4;
		unsigned : 1;
		unsigned : 3;
		unsigned m : 1;			/**< Mask. */
	} __attribute__ ((packed));
	uint64_t value;
};

typedef union cr_itv cr_itv_t;

/** Interruption Status Register */
union cr_isr {
	struct {
		union {
			/** General Exception code field structuring. */
			struct {
				unsigned ge_na : 4;
				unsigned ge_code : 4;
			} __attribute__ ((packed));
			uint16_t code;
		};
		uint8_t vector;
		unsigned : 8;
		unsigned x : 1;			/**< Execute exception. */
		unsigned w : 1;			/**< Write exception. */
		unsigned r : 1;			/**< Read exception. */
		unsigned na : 1;		/**< Non-access exception. */
		unsigned sp : 1;		/**< Speculative load exception. */
		unsigned rs : 1;		/**< Register stack. */
		unsigned ir : 1;		/**< Incomplete Register frame. */
		unsigned ni : 1;		/**< Nested Interruption. */
		unsigned so : 1;		/**< IA-32 Supervisor Override. */
		unsigned ei : 2;		/**< Excepting Instruction. */
		unsigned ed : 1;		/**< Exception Deferral. */
		unsigned : 20;
	} __attribute__ ((packed));
	uint64_t value;
};

typedef union cr_isr cr_isr_t;

/** CPUID Register 3 */
union cpuid3 {
	struct {
		uint8_t number;
		uint8_t revision;
		uint8_t model;
		uint8_t family;
		uint8_t archrev;
	} __attribute__ ((packed));
	uint64_t value;
};

typedef union cpuid3 cpuid3_t;

#endif /* !__ASM__ */

#endif

/** @}
 */
