/*
 * Copyright (c) 2026 Jakub Kodym
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
 * @brief Synopsys DesignWare DW_apb_uart driver.
 */

#ifndef KERN_DWAPB_H_
#define KERN_DWAPB_H_

#include <ddi/ddi.h>
#include <ddi/irq.h>
#include <typedefs.h>
#include <console/chardev.h>

#define BIT(n)            (1U << (n))

/** DW APB UART register map. */
typedef struct {
	union {
		/** Receiver Buffer Register (read). */
		const ioport32_t rbr;
		/** Transmitter Holding Register (write). */
		ioport32_t thr;
		/** Divisor Latch Low (DLAB=1). */
		ioport32_t dll;
	};
	union {
		/** Interrupt Enable Register (DLAB=0). */
		ioport32_t ier;
#define IER_ERBFI		BIT(0) /** Enable Received Data Available Interrupt.*/

		/** Divisor Latch High (DLAB=1). */
		ioport32_t dlh;
	};
	union {
		/** Interrupt Identification Register (read). */
		const ioport32_t iir;
#define IIR_IID_MASK	  0x0F   /** Mask for the Interrupt ID bits. */
#define IID_MODEM_STATUS  0x00	 /** Modem status. */
#define IID_NO_INTERRUPT  0x01 	 /** No interrupt pending. */
#define IID_THR_EMPTY     0x02 	 /** Transmitter Holding Register empty. */
#define IID_RECEIVED_DATA 0x04 	 /** Received data available. */
#define IID_LINE_STATUS   0x06 	 /** Receiver line status. */
#define IID_BUSY_DETECT   0x07 	 /** Busy detect. */
#define IID_CHAR_TIMEOUT  0x0C 	 /** Character timeout. */

		/** FIFO Control Register (write). */
		ioport32_t fcr;
	};
	/** Line Control Register. */
	ioport32_t lcr;
#define LCR_DLAB        BIT(7) /** Divisor Latch Access bit. */
#define LCR_SBE         BIT(6) /** Break Control Bit. */

	/** Modem Control Register. */
	ioport32_t mcr;
#define MCR_DTR			BIT(0) /** Data Terminal Ready. */
#define MCR_RTS  		BIT(1) /** Request to Send. */
#define MCR_OUT2 		BIT(3) /** OUT2. */

	/** Line Status Register. */
	const ioport32_t lsr;
#define LSR_DR   		BIT(0) /** Data ready in RBR. */
#define LSR_THRE 		BIT(5) /** Transmitter Holding Register empty. */

	/** Modem Status Register. */
	const ioport32_t msr;
	/** Scratchpad Register. */
	ioport32_t scr;
	/** Low Power Divisor Latch Low Register. */
	ioport32_t lpdll;
	/** Low Power Divisor Latch High Register. */
	ioport32_t lpdlh;
	/** Reserved. */
	uint32_t _padd_reserved[2];
	union {
		/** Shadow Receive Buffers (read). */
		const ioport32_t srbrn[16];
		/** Shadow Transmit Holding Registers (write). */
		ioport32_t sthrn[16];
	};
	/** FIFO Access Register. */
	ioport32_t far;
	/** Transmit FIFO Read. */
	const ioport32_t tfr;
	/** Receive FIFO Write. */
	ioport32_t rfw;
	/** UART Status Register. */
	const ioport32_t usr;
#define USR_BUSY  		BIT(0) /** UART busy. */

	/** Further registers are not used by the implementation. */
} dwapb_regs_t;

typedef struct {
	irq_t irq;
	dwapb_regs_t *regs;
	indev_t *input;
	outdev_t *output;
	parea_t parea;
} dwapb_instance_t;

extern dwapb_instance_t *dwapb_init(ioport32_t *, inr_t, cir_t, void *,
    outdev_t **);
extern void dwapb_wire(dwapb_instance_t *, indev_t *);

#endif /* KERN_DWAPB_H_ */

/** @}
 */
