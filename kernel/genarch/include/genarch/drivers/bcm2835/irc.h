/*
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
/** @addtogroup genarch
 * @{
 */
/**
 * @file
 * @brief Broadcom BCM2835 on-chip interrupt controller driver.
 */

#ifndef KERN_BCM2835_IRQC_H_
#define KERN_BCM2835_IRQC_H_

#include <typedefs.h>

#define BANK_GPU0	0
#define BANK_GPU1	1
#define BANK_ARM	2

#define IRQ_TO_BANK(x)	((x) >> 5)
#define IRQ_TO_NUM(x)	((x) & 0x1f)

#define MAKE_IRQ(b,n)	(((b) << 5) | ((n) & 0x1f))

#define BCM2835_UART_IRQ	MAKE_IRQ(BANK_GPU1, 25)
#define BCM2835_TIMER1_IRQ	MAKE_IRQ(BANK_GPU0,  1)

typedef struct {
	ioport32_t	irq_basic_pending;
	ioport32_t	irq_pending1;
	ioport32_t	irq_pending2;

	ioport32_t	fiq_control;

	ioport32_t	irq_enable[3];
	ioport32_t	irq_disable[3];
} bcm2835_irc_t;

#define BCM2835_IRC_ADDR 0x2000b200
#define BCM2835_IRQ_COUNT 96

static inline void bcm2835_irc_dump(bcm2835_irc_t *regs)
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

static inline void bcm2835_irc_init(bcm2835_irc_t *regs)
{
	/* Disable all interrupts */
	regs->irq_disable[BANK_GPU0] = 0xffffffff;
	regs->irq_disable[BANK_GPU1] = 0xffffffff;
	regs->irq_disable[BANK_ARM]  = 0xffffffff;

	/* Disable FIQ generation */
	regs->fiq_control = 0;
}

static inline int ffs(unsigned int x)
{
	int ret;

	asm volatile (
		"clz r0, %[x]\n"
		"rsb %[ret], r0, #32\n"
		: [ret] "=r" (ret)
		: [x] "r" (x)
		: "r0" );

	return ret;
}

static inline unsigned bcm2835_irc_inum_get(bcm2835_irc_t *regs)
{
	uint32_t pending;

	/* TODO: use 'shortcut' bits in basic pending register */

	pending = regs->irq_basic_pending;
	if (pending & 0xff)
		return MAKE_IRQ(BANK_ARM, ffs(pending & 0xFF) - 1);

	pending = regs->irq_pending1;
	if (pending)
		return MAKE_IRQ(BANK_GPU0, ffs(pending) - 1);

	pending = regs->irq_pending2;
	if (pending)
		return MAKE_IRQ(BANK_GPU1, ffs(pending) - 1);

	printf("Spurious interrupt!\n");
	bcm2835_irc_dump(regs);
	return 0;
}

static inline void bcm2835_irc_enable(bcm2835_irc_t *regs, unsigned inum)
{
	ASSERT(inum < BCM2835_IRQ_COUNT);
	regs->irq_enable[IRQ_TO_BANK(inum)] |= (1 << IRQ_TO_NUM(inum));
}

static inline void bcm2835_irc_disable(bcm2835_irc_t *regs, unsigned inum)
{
	ASSERT(inum < BCM2835_IRQ_COUNT);
	regs->irq_disable[IRQ_TO_BANK(inum)] |= (1 << IRQ_TO_NUM(inum));
}

#endif /* KERN_BCM2835_IRQC_H_ */

/**
 * @}
 */
