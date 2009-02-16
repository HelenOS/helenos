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

#include <console/chardev.h>
#include <ddi/irq.h>
#include <ipc/irq.h>

extern void ns16550_init(devno_t, uintptr_t, inr_t, cir_t, void *);
extern void ns16550_poll(void);
extern void ns16550_grab(void);
extern void ns16550_release(void);
extern char ns16550_key_read(chardev_t *);
extern irq_ownership_t ns16550_claim(void *);
extern void ns16550_irq_handler(irq_t *);

#include <arch/types.h>
#include <arch/drivers/kbd.h>

/* NS16550 registers */
#define RBR_REG		0	/** Receiver Buffer Register. */
#define IER_REG		1	/** Interrupt Enable Register. */
#define IIR_REG		2	/** Interrupt Ident Register (read). */
#define FCR_REG		2	/** FIFO control register (write). */
#define LCR_REG		3	/** Line Control register. */
#define MCR_REG		4	/** Modem Control Register. */
#define LSR_REG		5	/** Line Status Register. */

#define IER_ERBFI	0x01	/** Enable Receive Buffer Full Interrupt. */

#define LCR_DLAB	0x80	/** Divisor Latch Access bit. */

#define MCR_OUT2	0x08	/** OUT2. */

/** Structure representing the ns16550 device. */
typedef struct {
	devno_t devno;
	/** Memory mapped registers of the ns16550. */
	volatile ioport_t io_port;
} ns16550_t;

static inline uint8_t ns16550_rbr_read(ns16550_t *dev)
{
	return pio_read_8(dev->io_port + RBR_REG);
}
static inline void ns16550_rbr_write(ns16550_t *dev, uint8_t v)
{
	pio_write_8(dev->io_port + RBR_REG, v);
}

static inline uint8_t ns16550_ier_read(ns16550_t *dev)
{
	return pio_read_8(dev->io_port + IER_REG);
}

static inline void ns16550_ier_write(ns16550_t *dev, uint8_t v)
{
	pio_write_8(dev->io_port + IER_REG, v);
}

static inline uint8_t ns16550_iir_read(ns16550_t *dev)
{
	return pio_read_8(dev->io_port + IIR_REG);
}

static inline void ns16550_fcr_write(ns16550_t *dev, uint8_t v)
{
	pio_write_8(dev->io_port + FCR_REG, v);
}

static inline uint8_t ns16550_lcr_read(ns16550_t *dev)
{
	return pio_read_8(dev->io_port + LCR_REG);
}

static inline void ns16550_lcr_write(ns16550_t *dev, uint8_t v)
{
	pio_write_8(dev->io_port + LCR_REG, v);
}

static inline uint8_t ns16550_lsr_read(ns16550_t *dev)
{
	return pio_read_8(dev->io_port + LSR_REG);
}

static inline uint8_t ns16550_mcr_read(ns16550_t *dev)
{
	return pio_read_8(dev->io_port + MCR_REG);
}

static inline void ns16550_mcr_write(ns16550_t *dev, uint8_t v)
{
	pio_write_8(dev->io_port + MCR_REG, v);
}

#endif

/** @}
 */
