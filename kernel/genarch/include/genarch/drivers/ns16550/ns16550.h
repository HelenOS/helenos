/*
 * Copyright (c) 2001-2004 Jakub Jermar
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
 * @brief Headers for NS 16550 serial controller.
 */

#ifndef KERN_NS16550_H_
#define KERN_NS16550_H_

#include <ddi/ddi.h>
#include <ddi/irq.h>
#include <typedefs.h>
#include <console/chardev.h>

#define IER_ERBFI  0x01  /** Enable Receive Buffer Full Interrupt. */

#define LCR_DLAB   0x80  /** Divisor Latch Access bit. */

#define MCR_OUT2   0x08  /** OUT2. */

/** NS16550 registers. */
typedef enum {
	NS16550_REG_RBR = 0,  /**< Receiver Buffer Register (read). */
	NS16550_REG_THR = 0,  /**< Transmitter Holder Register (write). */
	NS16550_REG_IER = 1,  /**< Interrupt Enable Register. */
	NS16550_REG_IIR = 2,  /**< Interrupt Ident Register (read). */
	NS16550_REG_FCR = 2,  /**< FIFO control register (write). */
	NS16550_REG_LCR = 3,  /**< Line Control register. */
	NS16550_REG_MCR = 4,  /**< Modem Control Register. */
	NS16550_REG_LSR = 5,  /**< Line Status Register. */
} ns16550_reg_t;

/** Structure representing the ns16550 device. */
typedef struct {
	irq_t irq;
	volatile ioport8_t *ns16550;
	indev_t *input;
	outdev_t *output;
	parea_t parea;
	int reg_shift;
} ns16550_instance_t;

extern ns16550_instance_t *ns16550_init(ioport8_t *, unsigned, inr_t, cir_t,
    void *, outdev_t **);
extern void ns16550_wire(ns16550_instance_t *, indev_t *);

#endif

/** @}
 */
