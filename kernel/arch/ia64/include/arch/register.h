/*
 * Copyright (c) 2005 Jakub Jermar
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

#define DCR_PP_MASK  (1 << 0)
#define DCR_BE_MASK  (1 << 1)
#define DCR_LC_MASK  (1 << 2)
#define DCR_DM_MASK  (1 << 8)
#define DCR_DP_MASK  (1 << 9)
#define DCR_DK_MASK  (1 << 10)
#define DCR_DX_MASK  (1 << 11)
#define DCR_DR_MASK  (1 << 12)
#define DCR_DA_MASK  (1 << 13)
#define DCR_DD_MASK  (1 << 14)

#define CR_IVR_MASK  0x0f

#define PSR_IC_MASK   (1 << 13)
#define PSR_I_MASK    (1 << 14)
#define PSR_PK_MASK   (1 << 15)
#define PSR_DT_MASK   (1 << 17)
#define PSR_DFL_MASK  (1 << 18)
#define PSR_DFH_MASK  (1 << 19)
#define PSR_RT_MASK   (1 << 27)
#define PSR_IT_MASK   (1 << 36)

#define PSR_CPL_SHIFT         32
#define PSR_CPL_MASK_SHIFTED  3

#define PSR_RI_SHIFT  41
#define PSR_RI_LEN    2

#define PFM_MASK  (~0x3fffffffff)

#define RSC_MODE_MASK   3
#define RSC_PL_MASK     12

/** Application registers. */
#define AR_KR0       0
#define AR_KR1       1
#define AR_KR2       2
#define AR_KR3       3
#define AR_KR4       4
#define AR_KR5       5
#define AR_KR6       6
#define AR_KR7       7
/* ARs 8-15 are reserved */
#define AR_RSC       16
#define AR_BSP       17
#define AR_BSPSTORE  18
#define AR_RNAT      19
/* AR 20 is reserved */
#define AR_FCR       21
/* ARs 22-23 are reserved */
#define AR_EFLAG     24
#define AR_CSD       25
#define AR_SSD       26
#define AR_CFLG      27
#define AR_FSR       28
#define AR_FIR       29
#define AR_FDR       30
/* AR 31 is reserved */
#define AR_CCV       32
/* ARs 33-35 are reserved */
#define AR_UNAT      36
/* ARs 37-39 are reserved */
#define AR_FPSR      40
/* ARs 41-43 are reserved */
#define AR_ITC       44
/* ARs 45-47 are reserved */
/* ARs 48-63 are ignored */
#define AR_PFS       64
#define AR_LC        65
#define AR_EC        66
/* ARs 67-111 are reserved */
/* ARs 112-127 are ignored */

/** Control registers. */
#define CR_DCR   0
#define CR_ITM   1
#define CR_IVA   2
/* CR3-CR7 are reserved */
#define CR_PTA   8
/* CR9-CR15 are reserved */
#define CR_IPSR  16
#define CR_ISR   17
/* CR18 is reserved */
#define CR_IIP   19
#define CR_IFA   20
#define CR_ITIR  21
#define CR_IIPA  22
#define CR_IFS   23
#define CR_IIM   24
#define CR_IHA   25
/* CR26-CR63 are reserved */
#define CR_LID   64
#define CR_IVR   65
#define CR_TPR   66
#define CR_EOI   67
#define CR_IRR0  68
#define CR_IRR1  69
#define CR_IRR2  70
#define CR_IRR3  71
#define CR_ITV   72
#define CR_PMV   73
#define CR_CMCV  74
/* CR75-CR79 are reserved */
#define CR_LRR0  80
#define CR_LRR1  81
/* CR82-CR127 are reserved */

#ifndef __ASSEMBLER__

/** Processor Status Register. */
typedef union {
	uint64_t value;
	struct {
		unsigned int : 1;
		unsigned int be : 1;   /**< Big-Endian data accesses. */
		unsigned int up : 1;   /**< User Performance monitor enable. */
		unsigned int ac : 1;   /**< Alignment Check. */
		unsigned int mfl : 1;  /**< Lower floating-point register written. */
		unsigned int mfh : 1;  /**< Upper floating-point register written. */
		unsigned int : 7;
		unsigned int ic : 1;   /**< Interruption Collection. */
		unsigned int i : 1;    /**< Interrupt Bit. */
		unsigned int pk : 1;   /**< Protection Key enable. */
		unsigned int : 1;
		unsigned int dt : 1;   /**< Data address Translation. */
		unsigned int dfl : 1;  /**< Disabled Floating-point Low register set. */
		unsigned int dfh : 1;  /**< Disabled Floating-point High register set. */
		unsigned int sp : 1;   /**< Secure Performance monitors. */
		unsigned int pp : 1;   /**< Privileged Performance monitor enable. */
		unsigned int di : 1;   /**< Disable Instruction set transition. */
		unsigned int si : 1;   /**< Secure Interval timer. */
		unsigned int db : 1;   /**< Debug Breakpoint fault. */
		unsigned int lp : 1;   /**< Lower Privilege transfer trap. */
		unsigned int tb : 1;   /**< Taken Branch trap. */
		unsigned int rt : 1;   /**< Register Stack Translation. */
		unsigned int : 4;
		unsigned int cpl : 2;  /**< Current Privilege Level. */
		unsigned int is : 1;   /**< Instruction Set. */
		unsigned int mc : 1;   /**< Machine Check abort mask. */
		unsigned int it : 1;   /**< Instruction address Translation. */
		unsigned int id : 1;   /**< Instruction Debug fault disable. */
		unsigned int da : 1;   /**< Disable Data Access and Dirty-bit faults. */
		unsigned int dd : 1;   /**< Data Debug fault disable. */
		unsigned int ss : 1;   /**< Single Step enable. */
		unsigned int ri : 2;   /**< Restart Instruction. */
		unsigned int ed : 1;   /**< Exception Deferral. */
		unsigned int bn : 1;   /**< Register Bank. */
		unsigned int ia : 1;   /**< Disable Instruction Access-bit faults. */
	} __attribute__((packed));
} psr_t;

/** Register Stack Configuration Register */
typedef union {
	uint64_t value;
	struct {
		unsigned int mode : 2;
		unsigned int pl : 2;    /**< Privilege Level. */
		unsigned int be : 1;    /**< Big-endian. */
		unsigned int : 11;
		unsigned int loadrs : 14;
	} __attribute__((packed));
} rsc_t;

/** External Interrupt Vector Register */
typedef union {
	uint8_t vector;
	uint64_t value;
} cr_ivr_t;

/** Task Priority Register */
typedef union {
	uint64_t value;
	struct {
		unsigned int : 4;
		unsigned int mic : 4;  /**< Mask Interrupt Class. */
		unsigned int : 8;
		unsigned int mmi : 1;  /**< Mask Maskable Interrupts. */
	} __attribute__((packed));
} cr_tpr_t;

/** Interval Timer Vector */
typedef union {
	uint64_t value;
	struct {
		unsigned int vector : 8;
		unsigned int : 4;
		unsigned int : 1;
		unsigned int : 3;
		unsigned int m : 1;       /**< Mask. */
	} __attribute__((packed));
} cr_itv_t;

/** Interruption Status Register */
typedef union {
	uint64_t value;
	struct {
		union {
			/** General Exception code field structuring. */
			uint16_t code;
			struct {
				unsigned int ge_na : 4;
				unsigned int ge_code : 4;
			} __attribute__((packed));
		};
		uint8_t vector;
		unsigned int : 8;
		unsigned int x : 1;   /**< Execute exception. */
		unsigned int w : 1;   /**< Write exception. */
		unsigned int r : 1;   /**< Read exception. */
		unsigned int na : 1;  /**< Non-access exception. */
		unsigned int sp : 1;  /**< Speculative load exception. */
		unsigned int rs : 1;  /**< Register stack. */
		unsigned int ir : 1;  /**< Incomplete Register frame. */
		unsigned int ni : 1;  /**< Nested Interruption. */
		unsigned int so : 1;  /**< IA-32 Supervisor Override. */
		unsigned int ei : 2;  /**< Excepting Instruction. */
		unsigned int ed : 1;  /**< Exception Deferral. */
		unsigned int : 20;
	} __attribute__((packed));
} cr_isr_t;

/** CPUID Register 3 */
typedef union {
	uint64_t value;
	struct {
		uint8_t number;
		uint8_t revision;
		uint8_t model;
		uint8_t family;
		uint8_t archrev;
	} __attribute__((packed));
} cpuid3_t;

#endif /* !__ASSEMBLER__ */

#endif

/** @}
 */
