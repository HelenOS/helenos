/*
 * Copyright (c) 2012 Maurizio Lombardi
 *
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

/**
 * @file
 * @brief Texas Instruments OMAP on-chip interrupt controller driver.
 */

#ifndef KERN_OMAP_IRQC_H_
#define KERN_OMAP_IRQC_H_

#include <assert.h>
#include <typedefs.h>

#define OMAP_IRC_IRQ_GROUPS_PAD (4 - OMAP_IRC_IRQ_GROUPS_COUNT)

typedef struct {
	const ioport32_t revision;
#define OMAP_IRC_REV_MASK 0xFF

	const uint8_t padd0[12];

	/*
	 * This register controls the various parameters
	 * of the OCP interface.
	 */
	ioport32_t sysconfig;
#define OMAP_IRC_SYSCONFIG_AUTOIDLE_FLAG   (1 << 0)
#define OMAP_IRC_SYSCONFIG_SOFTRESET_FLAG  (1 << 1)

	/* This register provides status information about the module */
	const ioport32_t sysstatus;
#define OMAP_IRC_SYSSTATUS_RESET_DONE_FLAG (1 << 0)

	const uint8_t padd1[40];

	/* This register supplies the currently active IRQ interrupt number */
	ioport32_t sir_irq;
#define OMAP_IRC_SIR_IRQ_ACTIVEIRQ_MASK       0x7F
#define OMAP_IRC_SIR_IRQ_SPURIOUSIRQFLAG_MASK 0xFFFFFFF8

	/* This register supplies the currently active FIQ interrupt number */
	const ioport32_t sir_fiq;
#define OMAP_IRC_FIQ_IRQ_ACTIVEFIQ_MASK       0x7F
#define OMAP_IRC_FIQ_IRQ_SPURIOUSFIQFLAG_MASK 0xFFFFFFF8

	/* This register contains the new interrupt agreement bits */
	ioport32_t control;
#define OMAP_IRC_CONTROL_NEWIRQAGR_FLAG       (1 << 0)
#define OMAP_IRC_CONTROL_NEWFIQAGR_FLAG       (1 << 1)

	/*
	 * This register controls protection of the other registers.
	 * This register can only be accessed in priviledged mode, regardless
	 * of the current value of the protection bit.
	 */
	ioport32_t protection;
#define OMAP_IRC_PROTECTION_FLAG              (1 << 0)

	/*
	 * This register controls the clock auto-idle for the functional
	 * clock and the input synchronizers.
	 */
	ioport32_t idle;
#define OMAP_IRC_IDLE_FUNCIDLE_FLAG           (1 << 0)
#define OMAP_IRC_IDLE_TURBO_FLAG              (1 << 1)

	const uint8_t padd2[12];

	/* This register supplies the currently active IRQ priority level */
	const ioport32_t irq_priority;
#define OMAP_IRC_IRQ_PRIORITY_IRQPRIORITY_MASK     0x7F
#define OMAP_IRC_IRQ_PRIORITY_SPURIOUSIRQFLAG_MASK 0xFFFFFFF8

	/* This register supplies the currently active FIQ priority level */
	const ioport32_t fiq_priority;
#define OMAP_IRC_FIQ_PRIORITY_FIQPRIORITY_MASK     0x7F
#define OMAP_IRC_FIQ_PRIORITY_SPURIOUSIRQFLAG_MASK 0xFFFFFFF8

	/* This register sets the priority threshold */
	ioport32_t threshold;
#define OMAP_IRC_THRESHOLD_PRIORITYTHRESHOLD_MASK     0xFF
#define OMAP_IRC_THRESHOLD_PRIORITYTHRESHOLD_ENABLED  0x00
#define OMAP_IRC_THRESHOLD_PRIORITYTHRESHOLD_DISABLED 0xFF

	const uint8_t padd3[20];

	struct {
		/* Raw interrupt input status before masking */
		const ioport32_t itr;

		/* Interrupt mask */
		ioport32_t mir;

		/*
		 * This register is used to clear the interrupt mask bits,
		 * Write 1 clears the mask bit to 0.
		 */
		ioport32_t mir_clear;

		/*
		 * This register is used to set the interrupt mask bits,
		 * Write 1 sets the mask bit to 1.
		 */
		ioport32_t mir_set;

		/*
		 * This register is used to set the software interrupt bits,
		 * it is also used to read the current active software
		 * interrupts.
		 * Write 1 sets the software interrups bits to 1.
		 */
		ioport32_t isr_set;

		/*
		 * This register is used to clear the software interrups bits.
		 * Write 1 clears the software interrupt bits to 0.
		 */
		ioport32_t isr_clear;

		/* This register contains the IRQ status after masking. */
		const ioport32_t pending_irq;

		/* This register contains the FIQ status after masking. */
		const ioport32_t pending_fiq;
	} interrupts[OMAP_IRC_IRQ_GROUPS_COUNT];

	const uint32_t padd4[8 * OMAP_IRC_IRQ_GROUPS_PAD];

	/*
	 * These registers contain the priority for the interrups and
	 * the FIQ/IRQ steering.
	 */
	ioport32_t ilr[OMAP_IRC_IRQ_COUNT];

} omap_irc_regs_t;

/* 0 = Interrupt routed to IRQ, 1 = interrupt routed to FIQ */
#define OMAP_IRC_ILR_FIQNIRQ_FLAG    (1 << 0)
#define OMAP_IRC_ILR_PRIORITY_MASK   0x3F
#define OMAP_IRC_ILR_PRIORITY_SHIFT  2

static inline void omap_irc_init(omap_irc_regs_t *regs)
{
	int i;

	/* Initialization sequence */

	/*
	 * 1 - Program the SYSCONFIG register: if necessary, enable the
	 *     autogating by setting the AUTOIDLE bit.
	 */
	regs->sysconfig &= ~OMAP_IRC_SYSCONFIG_AUTOIDLE_FLAG;

	/*
	 * 2 - Program the IDLE register: if necessary, disable functional
	 *     clock autogating or enable synchronizer autogating by setting
	 *     the FUNCIDLE bit or the TURBO bit accordingly.
	 */
	regs->idle &= ~OMAP_IRC_IDLE_FUNCIDLE_FLAG;
	regs->idle &= ~OMAP_IRC_IDLE_TURBO_FLAG;

	/*
	 * 3 - Program ILRm register for each interrupt line: Assign a
	 *     priority level and set the FIQNIRQ bit for an FIQ interrupt
	 *     (by default, interrupts are mapped to IRQ and
	 *     priority is 0 (highest).
	 */

	for (i = 0; i < OMAP_IRC_IRQ_COUNT; ++i)
		regs->ilr[i] = 0;

	/*
	 * 4 - Program the MIRn register: Enable interrupts (by default,
	 *     all interrupt lines are masked).
	 */
	for (i = 0; i < OMAP_IRC_IRQ_GROUPS_COUNT; ++i)
		regs->interrupts[i].mir_set = 0xFFFFFFFF;
}

/** Get the currently active IRQ interrupt number
 *
 * @param regs     Pointer to the irc memory mapped registers
 *
 * @return         The active IRQ interrupt number
 */
static inline unsigned omap_irc_inum_get(omap_irc_regs_t *regs)
{
	return regs->sir_irq & OMAP_IRC_SIR_IRQ_ACTIVEIRQ_MASK;
}

/** Reset IRQ output and enable new IRQ generation
 *
 * @param regs    Pointer to the irc memory mapped registers
 */
static inline void omap_irc_irq_ack(omap_irc_regs_t *regs)
{
	regs->control = OMAP_IRC_CONTROL_NEWIRQAGR_FLAG;
}

/** Reset FIQ output and enable new FIQ generation
 *
 * @param regs    Pointer to the irc memory mapped registers
 */
static inline void omap_irc_fiq_ack(omap_irc_regs_t *regs)
{
	regs->control = OMAP_IRC_CONTROL_NEWFIQAGR_FLAG;
}

/** Clear an interrupt mask bit
 *
 * @param regs    Pointer to the irc memory mapped registers
 * @param inum    The interrupt to be enabled
 */
static inline void omap_irc_enable(omap_irc_regs_t *regs, unsigned inum)
{
	assert(inum < OMAP_IRC_IRQ_COUNT);
	const unsigned set = inum / 32;
	const unsigned pos = inum % 32;
	regs->interrupts[set].mir_clear = (1 << pos);
}

/** Set an interrupt mask bit
 *
 * @param regs    Pointer to the irc memory mapped registers
 * @param inum    The interrupt to be disabled
 */
static inline void omap_irc_disable(omap_irc_regs_t *regs, unsigned inum)
{
	assert(inum < OMAP_IRC_IRQ_COUNT);
	const unsigned set = inum / 32;
	const unsigned pos = inum % 32;
	regs->interrupts[set].mir_set = (1 << pos);
}

static inline void omap_irc_dump(omap_irc_regs_t *regs)
{
#define DUMP_REG(name) \
	printf("%s %p(%x).\n", #name, &regs->name, regs->name);

	DUMP_REG(revision);
	DUMP_REG(sysconfig);
	DUMP_REG(sysstatus);
	DUMP_REG(sir_irq);
	DUMP_REG(sir_fiq);
	DUMP_REG(control);
	DUMP_REG(protection);
	DUMP_REG(idle);
	DUMP_REG(irq_priority);
	DUMP_REG(fiq_priority);
	DUMP_REG(threshold);

	for (int i = 0; i < OMAP_IRC_IRQ_GROUPS_COUNT; ++i) {
		DUMP_REG(interrupts[i].itr);
		DUMP_REG(interrupts[i].mir);
		DUMP_REG(interrupts[i].isr_set);
		DUMP_REG(interrupts[i].pending_irq);
		DUMP_REG(interrupts[i].pending_fiq);
	}
	for (int i = 0; i < OMAP_IRC_IRQ_COUNT; ++i) {
		DUMP_REG(ilr[i]);
	}

#undef DUMP_REG
}

#endif

/**
 * @}
 */
