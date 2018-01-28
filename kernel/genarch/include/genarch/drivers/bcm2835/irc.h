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

#ifndef KERN_BCM2835_IRQC_H_
#define KERN_BCM2835_IRQC_H_

#include <assert.h>
#include <typedefs.h>

#define BANK_GPU0	0
#define BANK_GPU1	1
#define BANK_ARM	2

#define IRQ_TO_BANK(x)	((x) >> 5)
#define IRQ_TO_NUM(x)	((x) & 0x1f)

#define MAKE_IRQ(b,n)	(((b) << 5) | ((n) & 0x1f))

#define BCM2835_UART_IRQ	MAKE_IRQ(BANK_GPU1, 25)
#define BCM2835_TIMER1_IRQ	MAKE_IRQ(BANK_GPU0,  1)

#define IRQ_PEND_ARM_M		0xFF
#define IRQ_PEND_GPU0_M		(1 << 8)
#define IRQ_PEND_GPU1_M		(1 << 9)
#define IRQ_PEND_SHORT_M	0x1FFC00
#define IRQ_PEND_SHORT_S	10

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

extern void bcm2835_irc_init(bcm2835_irc_t *);
extern unsigned bcm2835_irc_inum_get(bcm2835_irc_t *);
extern void bcm2835_irc_enable(bcm2835_irc_t *, unsigned);
extern void bcm2835_irc_disable(bcm2835_irc_t *, unsigned);

#endif /* KERN_BCM2835_IRQC_H_ */

/**
 * @}
 */
