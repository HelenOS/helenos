/*
 * Copyright (c) 2012 Jan Vesely
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
 * @brief Texas Instruments AM/DM37x MPU on-chip interrupt controller driver.
 */

#ifndef KERN_AMDM37x_IRQC_H_
#define KERN_AMDM37x_IRQC_H_

/* AMDM37x TRM p. 1079 */
#define AMDM37x_IRC_BASE_ADDRESS 0x48200000
#define AMDM37x_IRC_SIZE 4096

#define AMDM37x_IRC_IRQ_COUNT 96

#include <typedefs.h>

typedef struct {
	const ioport32_t revision; /**< Revision */
#define AMDM37x_IRC_REV_MASK (0xff)

	uint8_t padd0_[12];

	ioport32_t sysconfig; /**< SYS config */
#define AMDM37x_IRC_SYSCONFIG_AUTOIDLE_FLAG (1 << 0)
#define AMDM37x_IRC_SYSCONFIG_SOFTRESET_FLAG (1 << 1)

	const ioport32_t sysstatus; /**< SYS status */
#define AMDM37x_IRC_SYSSTATUS_RESET_DONE_FLAG (1 << 0)

	uint8_t padd1_[40];

	const ioport32_t sir_irq;   /**< Currently active irq number */
#define AMDM37x_IRC_SIR_IRQ_ACTIVEIRQ_MASK (0x7f)
#define AMDM37x_IRC_SIR_IRQ_SPURIOUSIRQFLAG_MASK (0xfffffff8)

	const ioport32_t sir_fiq;
#define AMDM37x_IRC_SIR_FIQ_ACTIVEIRQ_MASK (0x7f)
#define AMDM37x_IRC_SIR_FIQ_SPURIOUSIRQFLAG_MASK (0xfffffff8)

	ioport32_t control;   /**< New interrupt agreement. */
#define AMDM37x_IRC_CONTROL_NEWIRQAGR_FLAG (1 << 0)
#define AMDM37x_IRC_CONTROL_NEWFIQAGR_FLAG (1 << 1)

	ioport32_t protection;  /**< Protect other registers. */
#define AMDM37x_IRC_PROTECTION_PROETCTION_FLAG (1 << 0)

	ioport32_t idle;   /**< Idle and autogating */
#define AMDM37x_IRC_IDLE_FUNCIDLE_FLAG (1 << 0)
#define AMDM37x_IRC_IDLE_TURBO_FLAG (1 << 1)

	uint8_t padd2_[12];

	ioport32_t irq_priority; /**< Active IRQ priority */
#define AMDM37x_IRC_IRQ_PRIORITY_IRQPRIORITY_MASK (0x7f)
#define AMDM37x_IRC_IRQ_PRIORITY_SPURIOUSIRQFLAG_MASK (0xfffffff8)

	ioport32_t fiq_priority; /**< Active FIQ priority */
#define AMDM37x_IRC_FIQ_PRIORITY_FIQPRIORITY_MASK (0x7f)
#define AMDM37x_IRC_FIQ_PRIORITY_SPURIOUSFIQFLAG_MASK (0xfffffff8)

	ioport32_t threshold; /**< Priority threshold */
#define AMDM37x_IRC_THRESHOLD_PRIORITYTHRESHOLD_MASK (0xff)
#define AMDM37x_IRC_THRESHOLD_PRIORITYTHRESHOLD_ENABLED (0x00)
#define AMDM37x_IRC_THRESHOLD_PRIORITYTHRESHOLD_DISABLED (0xff)

	uint8_t padd3__[20];

	struct {
		const ioport32_t itr;   /**< Interrupt input status before masking */
		ioport32_t mir;   /**< Interrupt mask */
		ioport32_t mir_clear; /**< Clear mir mask bits */
		ioport32_t mir_set;   /**< Set mir mask bits */
		ioport32_t isr_set;   /**< Set software interrupt bits */
		ioport32_t isr_clear; /**< Clear software interrupt bits */
		const ioport32_t pending_irq; /**< IRQ status after masking */
		const ioport32_t pending_fiq; /**< FIQ status after masking */
	} interrupts[3];

	uint8_t padd4_[32];

	ioport32_t ilr[96];   /**< FIQ/IRQ steering */
#define AMDM37x_IRC_ILR_FIQNIRQ (1 << 0)
#define AMDM37x_IRC_ILR_PRIORITY_MASK (0x3f)
#define AMDM37x_IRC_ILR_PRIORITY_SHIFT (2)

} amdm37x_irc_regs_t;

static inline void amdm37x_irc_dump(amdm37x_irc_regs_t *regs)
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

	for (int i = 0; i < 3; ++i) {
		DUMP_REG(interrupts[i].itr);
		DUMP_REG(interrupts[i].mir);
		DUMP_REG(interrupts[i].isr_set);
		DUMP_REG(interrupts[i].pending_irq);
		DUMP_REG(interrupts[i].pending_fiq);
	}
	for (int i = 0; i < AMDM37x_IRC_IRQ_COUNT; ++i) {
		DUMP_REG(ilr[i]);
	}

#undef DUMP_REG
}

static inline void amdm37x_irc_init(amdm37x_irc_regs_t *regs)
{
	/* AMDM37x TRM sec 12.5.1 p. 2425 */
	/* Program system config register */
	//TODO enable this when you know the meaning
	//regs->sysconfig |= AMDM37x_IRC_SYSCONFIG_AUTOIDLE_FLAG;

	/* Program idle register */
	//TODO enable this when you know the meaning
	//regs->sysconfig |= AMDM37x_IRC_IDLE_TURBO_FLAG;

	/* Program ilr[m] assign priority, decide fiq */
	for (unsigned i = 0; i < AMDM37x_IRC_IRQ_COUNT; ++i) {
		regs->ilr[i] = 0; /* highest prio(default) route to irq */
	}

	/* Disable all interrupts */
	regs->interrupts[0].mir_set = 0xffffffff;
	regs->interrupts[1].mir_set = 0xffffffff;
	regs->interrupts[2].mir_set = 0xffffffff;
}

static inline unsigned amdm37x_irc_inum_get(amdm37x_irc_regs_t *regs)
{
	return regs->sir_irq & AMDM37x_IRC_SIR_IRQ_ACTIVEIRQ_MASK;
}

static inline void amdm37x_irc_irq_ack(amdm37x_irc_regs_t *regs)
{
	regs->control = AMDM37x_IRC_CONTROL_NEWIRQAGR_FLAG;
}

static inline void amdm37x_irc_fiq_ack(amdm37x_irc_regs_t *regs)
{
	regs->control = AMDM37x_IRC_CONTROL_NEWFIQAGR_FLAG;
}

static inline void amdm37x_irc_enable(amdm37x_irc_regs_t *regs, unsigned inum)
{
	ASSERT(inum < AMDM37x_IRC_IRQ_COUNT);
	const unsigned set = inum / 32;
	const unsigned pos = inum % 32;
	regs->interrupts[set].mir_clear = (1 << pos);
}

static inline void amdm37x_irc_disable(amdm37x_irc_regs_t *regs, unsigned inum)
{
	ASSERT(inum < AMDM37x_IRC_IRQ_COUNT);
	const unsigned set = inum / 32;
	const unsigned pos = inum % 32;
	regs->interrupts[set].mir_set = (1 << pos);
}

#endif

/**
 * @}
 */
