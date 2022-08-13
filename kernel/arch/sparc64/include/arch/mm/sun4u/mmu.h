/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_sun4u_MMU_H_
#define KERN_sparc64_sun4u_MMU_H_

#if defined(US)
/* LSU Control Register ASI. */
#define ASI_LSU_CONTROL_REG		0x45	/**< Load/Store Unit Control Register. */
#endif

/* I-MMU ASIs. */
#define ASI_IMMU			0x50
#define ASI_IMMU_TSB_8KB_PTR_REG	0x51
#define ASI_IMMU_TSB_64KB_PTR_REG	0x52
#define ASI_ITLB_DATA_IN_REG		0x54
#define ASI_ITLB_DATA_ACCESS_REG	0x55
#define ASI_ITLB_TAG_READ_REG		0x56
#define ASI_IMMU_DEMAP			0x57

/* Virtual Addresses within ASI_IMMU. */
#define VA_IMMU_TSB_TAG_TARGET		0x0	/**< IMMU TSB tag target register. */
#define VA_IMMU_SFSR			0x18	/**< IMMU sync fault status register. */
#define VA_IMMU_TSB_BASE		0x28	/**< IMMU TSB base register. */
#define VA_IMMU_TAG_ACCESS		0x30	/**< IMMU TLB tag access register. */
#if defined (US3)
#define VA_IMMU_PRIMARY_EXTENSION	0x48	/**< IMMU TSB primary extension register */
#define VA_IMMU_NUCLEUS_EXTENSION	0x58	/**< IMMU TSB nucleus extension register */
#endif

/* D-MMU ASIs. */
#define ASI_DMMU			0x58
#define ASI_DMMU_TSB_8KB_PTR_REG	0x59
#define ASI_DMMU_TSB_64KB_PTR_REG	0x5a
#define ASI_DMMU_TSB_DIRECT_PTR_REG	0x5b
#define ASI_DTLB_DATA_IN_REG		0x5c
#define ASI_DTLB_DATA_ACCESS_REG	0x5d
#define ASI_DTLB_TAG_READ_REG		0x5e
#define ASI_DMMU_DEMAP			0x5f

/* Virtual Addresses within ASI_DMMU. */
#define VA_DMMU_TSB_TAG_TARGET		0x0	/**< DMMU TSB tag target register. */
#define VA_PRIMARY_CONTEXT_REG		0x8	/**< DMMU primary context register. */
#define VA_SECONDARY_CONTEXT_REG	0x10	/**< DMMU secondary context register. */
#define VA_DMMU_SFSR			0x18	/**< DMMU sync fault status register. */
#define VA_DMMU_SFAR			0x20	/**< DMMU sync fault address register. */
#define VA_DMMU_TSB_BASE		0x28	/**< DMMU TSB base register. */
#define VA_DMMU_TAG_ACCESS		0x30	/**< DMMU TLB tag access register. */
#define VA_DMMU_VA_WATCHPOINT_REG	0x38	/**< DMMU VA data watchpoint register. */
#define VA_DMMU_PA_WATCHPOINT_REG	0x40	/**< DMMU PA data watchpoint register. */
#if defined (US3)
#define VA_DMMU_PRIMARY_EXTENSION	0x48	/**< DMMU TSB primary extension register */
#define VA_DMMU_SECONDARY_EXTENSION	0x50	/**< DMMU TSB secondary extension register */
#define VA_DMMU_NUCLEUS_EXTENSION	0x58	/**< DMMU TSB nucleus extension register */
#endif

#ifndef __ASSEMBLER__

#include <arch/asm.h>
#include <barrier.h>
#include <stdint.h>

#if defined(US)
/** LSU Control Register. */
typedef union {
	uint64_t value;
	struct {
		unsigned : 23;
		unsigned pm : 8;
		unsigned vm : 8;
		unsigned pr : 1;
		unsigned pw : 1;
		unsigned vr : 1;
		unsigned vw : 1;
		unsigned : 1;
		unsigned fm : 16;
		unsigned dm : 1;	/**< D-MMU enable. */
		unsigned im : 1;	/**< I-MMU enable. */
		unsigned dc : 1;	/**< D-Cache enable. */
		unsigned ic : 1;	/**< I-Cache enable. */

	} __attribute__((packed));
} lsu_cr_reg_t;
#endif /* US */

#endif /* !def __ASSEMBLER__ */

#endif

/** @}
 */
