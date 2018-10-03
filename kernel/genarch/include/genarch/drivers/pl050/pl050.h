/*
 * Copyright (c) 2009 Vineeth Pillai
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
 * @brief	Describes the pl050 keyboard/mouse controller
 */

/**
 * This file implements pl050 specific functions for keyboard and mouse
 */

#ifndef KERN_genarch_PL050_H
#define KERN_genarch_PL050_H

#include <ddi/irq.h>
#include <console/chardev.h>
#include <typedefs.h>

/*
 * pl050 register offsets from the base address
 */
#define PL050_CR	0x00
#define PL050_STAT	0x04
#define PL050_DATA	0x08
#define PL050_CLOCKDIV	0x0C
#define PL050_INTRSTAT	0x10

/*
 * Control Register Bits
 */
#define PL050_CR_TYPE	(1 << 5)	/* Type 0: PS2/AT mode, 1: No Line control bit mode */
#define PL050_CR_RXINTR	(1 << 4)	/* Recieve Interrupt Enable */
#define PL050_CR_TXINTR	(1 << 3)	/* Transmit Interrupt Enable */
#define PL050_CR_INTR	(1 << 2)	/* Interrupt Enable */
#define PL050_CR_FKMID	(1 << 1)	/* Force KMI Data Low */
#define PL050_CR_FKMIC	1		/* Force KMI Clock Low */

/*
 * Status register bits
 */
#define PL050_STAT_TXEMPTY	(1 << 6)	/* 1: Transmit register empty */
#define PL050_STAT_TXBUSY	(1 << 5)	/* 1: Busy, sending data */
#define PL050_STAT_RXFULL	(1 << 4)	/* 1: register Full */
#define PL050_STAT_RXBUSY	(1 << 3)	/* 1: Busy, recieving Data */
#define PL050_STAT_RXPARITY	(1 << 2)	/* odd parity of the last bit received */
#define PL050_STAT_KMIC		(1 << 1)	/* status of KMICLKIN */
#define PL050_STAT_KMID		1		/* status of KMIDATAIN */

/*
 * Interrupt status register bits.
 */
#define PL050_TX_INTRSTAT	(1 << 1)	/* Transmit intr asserted */
#define PL050_RX_INTRSTAT	1		/* Recieve intr asserted */

typedef struct {
	ioport8_t *base;
	ioport8_t *data;
	ioport8_t *status;
	ioport8_t *ctrl;
} pl050_t;

typedef struct {
	irq_t	irq;
	pl050_t *pl050;
	indev_t *kbrdin;
} pl050_instance_t;

extern pl050_instance_t *pl050_init(pl050_t *, inr_t);
extern void pl050_wire(pl050_instance_t *, indev_t *);

#endif

/** @}
 */
