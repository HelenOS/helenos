/*
 * Copyright (c) 2015-2020 ARM Limited and Contributors.
 * Copyright (c) 2026 Jakub Kodym
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

/** @addtogroup kernel_genarch
 * @{
 */
/** @file
 * @brief ARM Generic Interrupt Controller v3 common declarations.
 */

#ifndef KERN_TFA_GIC_COMMON_H_
#define KERN_TFA_GIC_COMMON_H_

/*
 * GIC Distributor interface general definitions
 */

/* Constants to categorise interrupts */
#define MIN_SGI_ID		U(0)
#define MIN_SEC_SGI_ID		U(8)
#define MIN_PPI_ID		U(16)
#define MIN_SPI_ID		U(32)
#define MAX_SPI_ID		U(1019)

/* Mask for the priority field common to all GIC interfaces */
#define GIC_PRI_MASK			U(0xff)

/* Constant to indicate a spurious interrupt in all GIC versions */
#define GIC_SPURIOUS_INTERRUPT		U(1023)

/* Highest possible interrupt priorities */
#define GIC_HIGHEST_NS_PRIORITY		U(0x80)

/*
 * Common GIC Distributor interface register offsets
 */
#define GICD_CTLR		U(0x0)
#define GICD_TYPER		U(0x4)
#define GICD_IGROUPR		U(0x80)
#define GICD_ISENABLER		U(0x100)
#define GICD_ICENABLER		U(0x180)
#define GICD_IPRIORITYR		U(0x400)
#define GICD_ICFGR		U(0xc00)

/*
 * Common GIC Distributor interface register constants
 */

#define IGROUPR_SHIFT		5
#define ISENABLER_SHIFT		5
#define ICENABLER_SHIFT		ISENABLER_SHIFT
#define IPRIORITYR_SHIFT	2
#define ICFGR_SHIFT		4

/* GICD_TYPER shifts and masks */
#define TYPER_IT_LINES_NO_MASK	U(0x1f)

/* Value used to initialize Normal world interrupt priorities four at a time */
#define GICD_IPRIORITYR_DEF_VAL			\
	(GIC_HIGHEST_NS_PRIORITY	|	\
	(GIC_HIGHEST_NS_PRIORITY << 8)	|	\
	(GIC_HIGHEST_NS_PRIORITY << 16)	|	\
	(GIC_HIGHEST_NS_PRIORITY << 24))

#endif /* KERN_TFA_GIC_COMMON_H_ */

/** @}
 */
