/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 * SPDX-FileCopyrightText: 2012 Jan Vesely
 * SPDX-FileCopyrightText: 2013 Beniamino Galvani
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
