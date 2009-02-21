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
 * @brief	Headers for NS 16550 serial port / keyboard driver.
 */

#ifndef KERN_NS16550_H_
#define KERN_NS16550_H_

#include <ddi/irq.h>
#include <arch/types.h>
#include <arch/drivers/kbd.h>

#define IER_ERBFI	0x01	/** Enable Receive Buffer Full Interrupt. */

#define LCR_DLAB	0x80	/** Divisor Latch Access bit. */

#define MCR_OUT2	0x08	/** OUT2. */

/** NS16550 registers. */
struct ns16550 {
	ioport8_t rbr;	/**< Receiver Buffer Register. */
	ioport8_t ier;	/**< Interrupt Enable Register. */
	union {
		ioport8_t iir;	/**< Interrupt Ident Register (read). */
		ioport8_t fcr;	/**< FIFO control register (write). */
	} __attribute__ ((packed));
	ioport8_t lcr;	/**< Line Control register. */
	ioport8_t mcr;	/**< Modem Control Register. */
	ioport8_t lsr;	/**< Line Status Register. */
} __attribute__ ((packed));
typedef struct ns16550 ns16550_t;

/** Structure representing the ns16550 device. */
typedef struct ns16550_instance {
	devno_t devno;
	ns16550_t *ns16550;
	irq_t irq;
} ns16550_instance_t;

extern bool ns16550_init(ns16550_t *, devno_t, inr_t, cir_t, void *);
extern irq_ownership_t ns16550_claim(irq_t *);
extern void ns16550_irq_handler(irq_t *);

#endif

/** @}
 */
