/*
 * Copyright (c) 2016 Petr Pavlu
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

/** @addtogroup genarch
 * @{
 */
/** @file
 * @brief ARM Generic Interrupt Controller, Architecture version 2.0.
 */

#ifndef KERN_GICV2_H_
#define KERN_GICV2_H_

#include <typedefs.h>

/** GICv2 distributor register map. */
typedef struct {
	/** Distributor control register. */
	ioport32_t ctlr;
#define GICV2D_CTLR_ENABLE_FLAG  0x1

	/** Interrupt controller type register. */
	const ioport32_t typer;
#define GICV2D_TYPER_IT_LINES_NUMBER_SHIFT  0
#define GICV2D_TYPER_IT_LINES_NUMBER_MASK \
	(0x1f << GICV2D_TYPER_IT_LINES_NUMBER_SHIFT)

	/** Distributor implementer identification register. */
	const ioport32_t iidr;
	/** Reserved. */
	ioport32_t res_[5];
	/** Implementation defined registers. */
	ioport32_t impl[8];
	/** Reserved. */
	ioport32_t res2_[16];
	/** Interrupt group registers. */
	ioport32_t igroupr[32];
	/** Interrupt set-enable registers. */
	ioport32_t isenabler[32];
	/** Interrupt clear-enable registers. */
	ioport32_t icenabler[32];
	/** Interrupt set-pending registers. */
	ioport32_t ispendr[32];
	/** Interrupt clear-pending registers. */
	ioport32_t icpendr[32];
	/** GICv2 interrupt set-active registers. */
	ioport32_t isactiver[32];
	/** Interrupt clear-active registers. */
	ioport32_t icactiver[32];
	/** Interrupt priority registers. */
	ioport32_t ipriorityr[255];
	/** Reserved. */
	ioport32_t res3_;
	/** Interrupt processor target registers. First 8 words are read-only.
	 */
	ioport32_t itargetsr[255];
	/** Reserved. */
	ioport32_t res4_;
	/** Interrupt configuration registers. */
	ioport32_t icfgr[64];
	/** Implementation defined registers. */
	ioport32_t impl2[64];
	/** Non-secure access control registers. */
	ioport32_t nsacr[64];
	/** Software generated interrupt register. */
	ioport32_t sgir;
	/** Reserved. */
	ioport32_t res5_[3];
	/** SGI clear-pending registers. */
	ioport32_t cpendsgir[4];
	/** SGI set-pending registers. */
	ioport32_t spendsgir[4];
	/** Reserved. */
	ioport32_t res6_[40];
	/** Implementation defined identification registers. */
	const ioport32_t impl3[12];
} gicv2_distr_regs_t;

/* GICv2 CPU interface register map. */
typedef struct {
	/** CPU interface control register. */
	ioport32_t ctlr;
#define GICV2C_CTLR_ENABLE_FLAG  0x1

	/** Interrupt priority mask register. */
	ioport32_t pmr;
	/** Binary point register. */
	ioport32_t bpr;
	/** Interrupt acknowledge register. */
	const ioport32_t iar;
#define GICV2C_IAR_INTERRUPT_ID_SHIFT  0
#define GICV2C_IAR_INTERRUPT_ID_MASK \
	(0x3ff << GICV2C_IAR_INTERRUPT_ID_SHIFT)
#define GICV2C_IAR_CPUID_SHIFT  10
#define GICV2C_IAR_CPUID_MASK \
	(0x7 << GICV2C_IAR_CPUID_SHIFT)

	/** End of interrupt register. */
	ioport32_t eoir;
	/** Running priority register. */
	const ioport32_t rpr;
	/** Highest priority pending interrupt register. */
	const ioport32_t hppir;
	/** Aliased binary point register. */
	ioport32_t abpr;
	/** Aliased interrupt acknowledge register. */
	const ioport32_t aiar;
	/** Aliased end of interrupt register. */
	ioport32_t aeoir;
	/** Aliased highest priority pending interrupt register. */
	const ioport32_t ahppir;
	/** Reserved. */
	ioport32_t res_[5];
	/** Implementation defined registers. */
	ioport32_t impl[36];
	/** Active priorities registers. */
	ioport32_t apr[4];
	/** Non-secure active priorities registers. */
	ioport32_t nsapr[4];
	/** Reserved. */
	ioport32_t res2_[3];
	/** CPU interface identification register. */
	const ioport32_t iidr;
	/** Unallocated. */
	ioport32_t unalloc_[960];
	/** Deactivate interrupt register. */
	ioport32_t dir;
} gicv2_cpui_regs_t;

/** GICv2 driver-specific device data. */
typedef struct {
	gicv2_distr_regs_t *distr;
	gicv2_cpui_regs_t *cpui;
	unsigned inum_total;
} gicv2_t;

extern void gicv2_init(gicv2_t *, gicv2_distr_regs_t *, gicv2_cpui_regs_t *);
extern unsigned gicv2_inum_get_total(gicv2_t *);
extern void gicv2_inum_get(gicv2_t *, unsigned *, unsigned *);
extern void gicv2_end(gicv2_t *, unsigned, unsigned);
extern void gicv2_enable(gicv2_t *, unsigned);
extern void gicv2_disable(gicv2_t *, unsigned);

#endif

/** @}
 */
