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

/** @addtogroup uspace_drv_dwapb
 * @{
 */
/** @file
 * @brief Synopsys DesignWare DW_apb_uart driver.
 */

#include <errno.h>
#include <io/chardev_srv.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <macros.h>

#include "dwapb.h"

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

static void dwapb_connection(ipc_call_t *, void *);

static errno_t dwapb_read(chardev_srv_t *, void *, size_t, size_t *,
    chardev_flags_t);
static errno_t dwapb_write(chardev_srv_t *, const void *, size_t, size_t *);

static chardev_ops_t dwapb_chardev_ops = {
	.read = dwapb_read,
	.write = dwapb_write
};

/** Address range accessed by the DWAPB interrupt pseudo-code. */
static const irq_pio_range_t dwapb_ranges_proto[] = {
	{
		.base = 0,
		.size = sizeof(dwapb_regs_t)
	}
};

/** DWAPB interrupt pseudo-code instructions. */
static const irq_cmd_t dwapb_cmds_proto[] = {
	/* 0 */
	{
		/* Read Interrupt Identification Register. */
		.cmd = CMD_PIO_READ_32,
		.addr = NULL, /* Filled in runtime. */
		.dstarg = 1
	},
	/* 1 */
	{
		/* Mask Interrupt ID bits */
		.cmd = CMD_AND,
		.value = UINT16_C(IIR_IID_MASK),
		.srcarg = 1,
		.dstarg = 1
	},
	/* 2 */
	{
		/* Skip if Modem Status interrupt. */
		.cmd = CMD_PREDICATE,
		.value = UINT32_C(14),
		.srcarg = 1
	},
	/* 3 */
	{
		.cmd = CMD_AND,
		.value = UINT32_C(0x01),
		.srcarg = 1,
		.dstarg = 3
	},
	/* 4 */
	{
		/*
		 * Skip if Transmitter Holding Register Empty, Received Data, Line
		 * Status or Character Timeout.
		 */
		.cmd = CMD_PREDICATE,
		.value = UINT32_C(4),
		.srcarg = 3
	},
	/* 5 */
	/* Busy Detect or No Interrupt. */
	{
		.cmd = CMD_AND,
		.value = ~UINT32_C(0x01),
		.srcarg = 1,
		.dstarg = 3
	},
	/* 6 */
	{
		/* Skip if No Interrupt. */
		.cmd = CMD_PREDICATE,
		.value = UINT32_C(1),
		.srcarg = 3
	},
	/* 7 */
	{
		/* Busy Detect. */
		/* Read UART Status Register. */
		.cmd = CMD_PIO_READ_32,
		.addr = NULL, /* Filled in runtime. */
		.dstarg = 2
	},
	/* 8 */
	/* No Interrupt does not need clearing. */
	{
		.cmd = CMD_ACCEPT
	},
	/* 9 */
	{
		/*  */
		.cmd = CMD_AND,
		.value = UINT32_C(0x02),
		.srcarg = 1,
		.dstarg = 3
	},
	/* 10 */
	{
		/* Skip if Received Data or Character Timeout. */
		.cmd = CMD_PREDICATE,
		.value = UINT32_C(4),
		.srcarg = 3
	},
	/* 11 */
	{
		/*  */
		.cmd = CMD_AND,
		.value = UINT32_C(0x04),
		.srcarg = 1,
		.dstarg = 3
	},
	/* 12 */
	{
		/* Skip if Transmitter Holding Register Empty. */
		.cmd = CMD_PREDICATE,
		.value = UINT32_C(1),
		.srcarg = 3
	},
	/* 13 */
	{
		/* Receiver Line Status. */
		/* Read Line Status Register. */
		.cmd = CMD_PIO_READ_32,
		.addr = NULL, /* Filled in runtime. */
		.dstarg = 2
	},
	/* 14 */
	/* THRE has already been cleared by reading IIR. */
	{
		.cmd = CMD_ACCEPT
	},
	/* 15 */
	/* Received Data or Character Timeout. */
	{
		/* Read Receiver Buffer Register. */
		.cmd = CMD_PIO_READ_32,
		.addr = NULL, /* Filled in runtime. */
		.dstarg = 2
	},
	/* 16 */
	{
		.cmd = CMD_ACCEPT
	},
	/* 17 */
	/* Modem Status. */
	{
		/* Read Modem Status Register. */
		.cmd = CMD_PIO_READ_32,
		.addr = NULL, /* Filled in runtime. */
		.dstarg = 2
	},
	/* 18 */
	{
		.cmd = CMD_ACCEPT
	},
};

#define BIT(n)      (1U << (n))

#define RETRY_CNT	100000

#define NOTHING \
	do { \
	} while(0)

#define WHILE_CNT_AND_COND_DO(cnt, expr, statement) \
	do { \
		for (volatile unsigned i = 0; i < (cnt); i++) \
			if ((expr)) \
				statement; \
			else \
				break; \
	} while (0)

/* Wait until the UART is not busy. */
static void dwapb_wait_busy(dwapb_regs_t *regs)
{
	/* If not implemented, the register reads as zero. */
	WHILE_CNT_AND_COND_DO(RETRY_CNT,
	    pio_read_32(&regs->usr) & USR_BUSY,
	    NOTHING);
}

/** Initialize the DW_APB_UART device. */
static void dwapb_device_init(dwapb_t *instance)
{
	dwapb_regs_t *regs = instance->regs;

	/*
	 * We need to turn the FIFO off because of the uspace interrupt
	 * handling.
	 */
	/* The FIFO can be disabled only for both transmitting and receiving. */

	dwapb_wait_busy(regs);

	pio_write_32(&regs->fcr, 0x06);
	dwapb_wait_busy(regs);

	pio_write_32(&regs->fcr, 0);
	dwapb_wait_busy(regs);
}

/** Process an interrupt from a DWAPB device. */
static void dwapb_irq_handler(ipc_call_t *call, void *arg)
{
	dwapb_t *dwapb = (dwapb_t *) arg;
	circ_buf_t *cbufp = &dwapb->cbuf;
	uint32_t iid = ipc_get_arg1(call);
	uint8_t c = ipc_get_arg2(call);
	errno_t rc;

	if (iid != IID_RECEIVED_DATA && iid != IID_CHAR_TIMEOUT) {
		/* No handling. */
		return;
	}

	fibril_mutex_lock(&dwapb->buf_lock);

	rc = circ_buf_push(cbufp, &c);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Buffer overrun");
		goto end;
	}

end:
	fibril_mutex_unlock(&dwapb->buf_lock);
	fibril_condvar_broadcast(&dwapb->buf_cv);
}

/** Add a DWAPB device. */
errno_t dwapb_add(dwapb_t *dwapb, dwapb_res_t *res)
{
	ddf_fun_t *fun = NULL;
	irq_pio_range_t *dwapb_ranges = NULL;
	irq_cmd_t *dwapb_cmds = NULL;
	errno_t rc;

	circ_buf_init(&dwapb->cbuf, dwapb->buf, dwapb_buf_size, 1);
	fibril_mutex_initialize(&dwapb->buf_lock);
	fibril_condvar_initialize(&dwapb->buf_cv);

	dwapb->irq_handle = CAP_NIL;

	dwapb_ranges = malloc(sizeof(dwapb_ranges_proto));
	if (dwapb_ranges == NULL) {
		rc = ENOMEM;
		goto error;
	}
	dwapb_cmds = malloc(sizeof(dwapb_cmds_proto));
	if (dwapb_cmds == NULL) {
		rc = ENOMEM;
		goto error;
	}

	dwapb->res = *res;

	fun = ddf_fun_create(dwapb->dev, fun_exposed, "a");
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Error creating function 'a'.");
		rc = ENOMEM;
		goto error;
	}

	rc = pio_enable((void *)res->base, sizeof(dwapb_regs_t),
	    (void **) &dwapb->regs);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error enabling I/O");
		goto error;
	}

	ddf_fun_set_conn_handler(fun, dwapb_connection);

	memcpy(dwapb_ranges, dwapb_ranges_proto, sizeof(dwapb_ranges_proto));
	memcpy(dwapb_cmds, dwapb_cmds_proto, sizeof(dwapb_cmds_proto));
	dwapb_ranges[0].base = res->base;
	dwapb_regs_t *regsphys = (dwapb_regs_t *) res->base;

	dwapb_cmds[0].addr = (void *) &regsphys->iir;
	dwapb_cmds[7].addr = (void *) &regsphys->usr;
	dwapb_cmds[13].addr = (void *) &regsphys->lsr;
	dwapb_cmds[15].addr = (void *) &regsphys->rbr;
	dwapb_cmds[17].addr = (void *) &regsphys->msr;

	dwapb->irq_code.rangecount =
	    sizeof(dwapb_ranges_proto) / sizeof(irq_pio_range_t);
	dwapb->irq_code.ranges = dwapb_ranges;
	dwapb->irq_code.cmdcount = sizeof(dwapb_cmds_proto) / sizeof(irq_cmd_t);
	dwapb->irq_code.cmds = dwapb_cmds;

	rc = async_irq_subscribe(res->irq, dwapb_irq_handler, dwapb,
	    &dwapb->irq_code, &dwapb->irq_handle);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error registering IRQ code.");
		goto error;
	}

	chardev_srvs_init(&dwapb->cds);
	dwapb->cds.ops = &dwapb_chardev_ops;
	dwapb->cds.sarg = dwapb;

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error binding function 'a'.");
		goto error;
	}

	rc = ddf_fun_add_to_category(fun, "console");
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error adding function 'a' to category "
		    "'console'.");
		goto error;
	}

	/* Initialize the device for use in uspace. */
	dwapb_device_init(dwapb);

	return EOK;
error:
	if (cap_handle_valid(dwapb->irq_handle))
		async_irq_unsubscribe(dwapb->irq_handle);
	if (fun != NULL)
		ddf_fun_destroy(fun);
	free(dwapb_ranges);
	free(dwapb_cmds);

	return rc;
}

/** Remove DWAPB device */
errno_t dwapb_remove(dwapb_t *dwapb)
{
	return ENOTSUP;
}

/** DWAPB device gone */
errno_t dwapb_gone(dwapb_t *dwapb)
{
	return ENOTSUP;
}

/** Send a character to a DWAPB device.
 *
 * @param ch Character to be printed.
 */
static void dwapb_putchar(dwapb_t *dwapb, uint8_t ch)
{
	dwapb_regs_t *regs = (dwapb_regs_t *)dwapb->regs;

	/* Wait until the transmitter is ready. */
	while (!(pio_read_32(&regs->lsr) & LSR_THRE))
		;

	pio_write_32(&regs->thr, (uint32_t)ch);
}

/** Read from a DWAPB device. */
static errno_t dwapb_read(chardev_srv_t *srv, void *buf, size_t size,
    size_t *nread, chardev_flags_t flags)
{
	dwapb_t *dwapb = (dwapb_t *) srv->srvs->sarg;
	size_t p;
	uint8_t *bp = (uint8_t *) buf;
	errno_t rc;

	fibril_mutex_lock(&dwapb->buf_lock);

	while ((flags & chardev_f_nonblock) == 0 &&
	    circ_buf_nused(&dwapb->cbuf) == 0)
		fibril_condvar_wait(&dwapb->buf_cv, &dwapb->buf_lock);
	p = 0;
	while (p < size) {
		rc = circ_buf_pop(&dwapb->cbuf, &bp[p]);
		if (rc != EOK)
			break;
		++p;
	}

	fibril_mutex_unlock(&dwapb->buf_lock);

	*nread = p;
	return EOK;
}

/** Write to a DWAPB device. */
static errno_t dwapb_write(chardev_srv_t *srv, const void *data, size_t size,
    size_t *nwr)
{
	dwapb_t *dwapb = (dwapb_t *) srv->srvs->sarg;
	size_t i;
	uint8_t *dp = (uint8_t *) data;

	for (i = 0; i < size; i++)
		dwapb_putchar(dwapb, dp[i]);

	*nwr = size;
	return EOK;
}

/** Character device connection handler. */
static void dwapb_connection(ipc_call_t *icall, void *arg)
{
	dwapb_t *dwapb = (dwapb_t *) ddf_dev_data_get(
	    ddf_fun_get_dev((ddf_fun_t *) arg));

	chardev_conn(icall, &dwapb->cds);
}

/** @}
 */
