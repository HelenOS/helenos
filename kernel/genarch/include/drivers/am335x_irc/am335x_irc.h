/*
 * Copyright (c) 2012 Maurizio Lombardi
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

/**
 * @file
 * @brief Texas Instruments AM335x MPU on-chip interrupt controller driver.
 */

#ifndef KERN_AM335x_IRQC_H_
#define KERN_AM335x_IRQC_H_

#define AM335x_IRC_BASE_ADDRESS 0x48200000
#define AM335x_IRC_SIZE         4096

#define AM335x_IRC_IRQ_COUNT    128

#include <typedefs.h>

typedef struct {
	const ioport32_t revision;
#define AM335x_IRC_REV_MASK 0xFF

	const uint8_t padd0[12];

	ioport32_t sysconfig;
#define AM335x_IRC_SYSCONFIG_AUTOIDLE_FLAG   (1 << 0)
#define AM335x_IRC_SYSCONFIG_SOFTRESET_FLAG  (1 << 1)

	const ioport32_t sysstatus;
#define AM335x_IRC_SYSSTATUS_RESET_DONE_FLAG (1 << 0)

	const uint8_t padd1[40];

	ioport32_t sir_irq;
#define AM335x_IRC_SIR_IRQ_ACTIVEIRQ_MASK       0x7F
#define AM335x_IRC_SIR_IRQ_SPURIOUSIRQFLAG_MASK 0xFFFFFFF8

	const ioport32_t sir_fiq;
#define AM335x_IRC_FIQ_IRQ_ACTIVEFIQ_MASK       0x7F
#define AM335x_IRC_FIQ_IRQ_SPURIOUSFIQFLAG_MASK 0xFFFFFFF8

	ioport32_t control; /* New IRQ/FIQ agreement */
#define AM335x_IRC_CONTROL_NEWIRQAGR_FLAG       (1 << 0)
#define AM335x_IRC_CONTROL_NEWFIQAGR_FLAG       (1 << 1)

	ioport32_t protection;
#define AM335x_IRC_PROTECTION_FLAG              (1 << 0)

	ioport32_t idle;
#define AM335x_IRC_IDLE_FUNCIDLE_FLAG           (1 << 0)
#define AM335x_IRC_IDLE_TURBO_FLAG              (1 << 1)

	const uint8_t padd2[12];

	const ioport32_t irq_priority;
#define AM335x_IRC_IRQ_PRIORITY_IRQPRIORITY_MASK     0x7F
#define AM335x_IRC_IRQ_PRIORITY_SPURIOUSIRQFLAG_MASK 0xFFFFFFF8

	const ioport32_t fiq_priority;
#define AM335x_IRC_FIQ_PRIORITY_FIQPRIORITY_MASK     0x7F
#define AM335x_IRC_FIQ_PRIORITY_SPURIOUSIRQFLAG_MASK 0xFFFFFFF8

	ioport32_t threshold;
#define AM335x_IRC_THRESHOLD_PRIORITYTHRESHOLD_MASK     0xFF
#define AM335x_IRC_THRESHOLD_PRIORITYTHRESHOLD_ENABLED  0x00
#define AM335x_IRC_THRESHOLD_PRIORITYTHRESHOLD_DISABLED 0xFF

	const uint8_t padd[20];

	struct {
		/* Raw interrupt input status before masking */
		const ioport32_t itr;

		/* Interrupt mask */
		ioport32_t mir;

		/* This register is used to clear the interrupt mask bits,
		 * Write 1 clears the mask bit to 0.
		 */
		ioport32_t mir_clear;

		/* This register is used to set the interrupt mask bits,
		 * Write 1 sets the mask bit to 1.
		 */
		ioport32_t mir_set;

		/* This register is used to set the software interrupt bits,
		 * it is also used to read the current active software
		 * interrupts.
		 * Write 1 sets the software interrups bits to 1.
		 */
		ioport32_t isr_set;

		/* This register is used to clear the software interrups bits.
		 * Write 1 clears the software interrupt bits to 0.
		 */
		ioport32_t isr_clear;

		/* This register contains the IRQ status after masking. */
		const ioport32_t pending_irq;

		/* This register contains the FIQ status after masking. */
		const ioport32_t pending_fiq;
	} interrupts[4];

	/* These registers contain the priority for the interrups and
	 * the FIQ/IRQ steering.
	 */
	ioport32_t ilr[AM335x_IRC_IRQ_COUNT];
/* 0 = Interrupt routed to IRQ, 1 = interrupt routed to FIQ */
#define AM335x_IRC_ILR_FIQNIRQ_FLAG    (1 << 0)
#define AM335x_IRC_ILR_PRIORITY_MASK   0x3F
#define AM335x_IRC_ILR_PRIORITY_SHIFT  2

} am335x_irc_regs_t;

static inline void am335x_irc_init(am335x_irc_regs_t *regs)
{
	int i;

	/* Initialization sequence */

	/* 1 - Program the SYSCONFIG register: if necessary, enable the
	 *     autogating by setting the AUTOIDLE bit.
	 */
	regs->sysconfig &= ~AM335x_IRC_SYSCONFIG_AUTOIDLE_FLAG;

	/* 2 - Program the IDLE register: if necessary, disable functional
	 *     clock autogating or enable synchronizer autogating by setting
	 *     the FUNCIDLE bit or the TURBO bit accordingly.
	 */
	regs->idle &= ~AM335x_IRC_IDLE_FUNCIDLE_FLAG;
	regs->idle &= ~AM335x_IRC_IDLE_TURBO_FLAG;

	/* 3 - Program ILRm register for each interrupt line: Assign a
	 *     priority level and set the FIQNIRQ bit for an FIQ interrupt
	 *     (by default, interrupts are mapped to IRQ and
	 *     priority is 0 (highest).
	 */

	for (i = 0; i < AM335x_IRC_IRQ_COUNT; ++i)
		regs->ilr[i] = 0;

	/* 4 - Program the MIRn register: Enable interrupts (by default,
	 *     all interrupt lines are masked).
	 */
	for (i = 0; i < 4; ++i)
		regs->interrupts[i].mir_set = 0xFFFFFFFF;
}

#endif

/**
 * @}
 */
