/*
 * Copyright (c) 2019 Jiri Svoboda
 * Copyright (c) 2012 Jan Vesely
 * Copyright (c) 2013 Beniamino Galvani
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
 * @brief Broadcom BCM2835 on-chip interrupt controller driver.
 */

#include <assert.h>
#include <genarch/drivers/bcm2835/irc.h>
#include <stdio.h>

unsigned shortcut_inums[] = { 7, 9, 10, 18, 19, 53, 54, 55, 56, 57, 62 };

static void bcm2835_irc_dump(bcm2835_irc_t *regs)
{
#define DUMP_REG(name) \
	printf("%s : %08x\n", #name, regs->name);

	DUMP_REG(irq_basic_pending);
	DUMP_REG(irq_pending1);
	DUMP_REG(irq_pending2);
	DUMP_REG(fiq_control);

	for (int i = 0; i < 3; ++i) {
		DUMP_REG(irq_enable[i]);
		DUMP_REG(irq_disable[i]);
	}
#undef DUMP_REG
}

void bcm2835_irc_init(bcm2835_irc_t *regs)
{
	/* Disable all interrupts */
	regs->irq_disable[BANK_GPU0] = 0xffffffff;
	regs->irq_disable[BANK_GPU1] = 0xffffffff;
	regs->irq_disable[BANK_ARM]  = 0xffffffff;

	/* Disable FIQ generation */
	regs->fiq_control = 0;
}

static int ffs(unsigned int x)
{
	int ret;

	asm volatile (
	    "clz r0, %[x]\n"
	    "rsb %[ret], r0, #32\n"
	    : [ret] "=r" (ret)
	    : [x] "r" (x)
	    : "r0"
	);

	return ret;
}

unsigned bcm2835_irc_inum_get(bcm2835_irc_t *regs)
{
	uint32_t pending;
	int inum = -1;

	pending = regs->irq_basic_pending;

	/*
	 * The basic pending register shows interrupts pending from ARM
	 * peripherals and it also contains, in order to speed up processing,
	 * additional information about pending GPU interrupts:
	 *
	 *  - bits 0-7 are associated to ARM peripherals
	 *  - bit 8 is 1 when at least one bit is set in pending register 1
	 *  - bit 9 is 1 when at least one bit is set in pending register 2
	 *  - bits 10-20 indicate pending status of selected GPU peripherals
	 *
	 *  Reference: BCM2835 ARM Peripherals, p.113
	 */

	if (pending & IRQ_PEND_ARM_M) {
		inum = MAKE_IRQ(BANK_ARM, ffs(pending & IRQ_PEND_ARM_M) - 1);
	} else if (pending & IRQ_PEND_SHORT_M) {
		int pos = (pending & IRQ_PEND_SHORT_M) >> IRQ_PEND_SHORT_S;
		inum = shortcut_inums[ffs(pos) - 1];
	} else if (pending & IRQ_PEND_GPU0_M) {
		inum = MAKE_IRQ(BANK_GPU0, ffs(regs->irq_pending1) - 1);
	} else if (pending & IRQ_PEND_GPU1_M) {
		inum = MAKE_IRQ(BANK_GPU1, ffs(regs->irq_pending2) - 1);
	}

	if (inum < 0) {
		printf("Spurious interrupt!\n");
		bcm2835_irc_dump(regs);
		inum = 0;
	}

	return inum;
}

void bcm2835_irc_enable(bcm2835_irc_t *regs, unsigned inum)
{
	assert(inum < BCM2835_IRQ_COUNT);
	regs->irq_enable[IRQ_TO_BANK(inum)] |= (1 << IRQ_TO_NUM(inum));
}

void bcm2835_irc_disable(bcm2835_irc_t *regs, unsigned inum)
{
	assert(inum < BCM2835_IRQ_COUNT);
	regs->irq_disable[IRQ_TO_BANK(inum)] |= (1 << IRQ_TO_NUM(inum));
}

/**
 * @}
 */
