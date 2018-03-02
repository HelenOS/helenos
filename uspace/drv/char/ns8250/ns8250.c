/*
 * Copyright (c) 2010 Lenka Trochtova
 * Copyright (c) 2017 Jiri Svoboda
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

/**
 * @defgroup ns8250 Serial port driver.
 * @brief HelenOS serial port driver.
 * @{
 */

/** @file
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <str_error.h>
#include <stdbool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <str.h>
#include <ctype.h>
#include <macros.h>
#include <stdlib.h>
#include <dirent.h>
#include <ddi.h>

#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <ddf/log.h>
#include <io/chardev_srv.h>

#include <device/hw_res.h>
#include <ipc/serial_ctl.h>

#include "cyclic_buffer.h"

#define NAME "ns8250"

#define REG_COUNT 7
#define MAX_BAUD_RATE 115200
#define DLAB_MASK (1 << 7)

/** Interrupt Enable Register definition. */
#define	NS8250_IER_RXREADY	(1 << 0)
#define	NS8250_IER_THRE		(1 << 1)
#define	NS8250_IER_RXSTATUS	(1 << 2)
#define	NS8250_IER_MODEM_STATUS	(1 << 3)

/** Interrupt ID Register definition. */
#define	NS8250_IID_ACTIVE	(1 << 0)
#define NS8250_IID_CAUSE_MASK 0x0e
#define NS8250_IID_CAUSE_RXSTATUS 0x06

/** FIFO Control Register definition. */
#define	NS8250_FCR_FIFOENABLE	(1 << 0)
#define	NS8250_FCR_RXFIFORESET	(1 << 1)
#define	NS8250_FCR_TXFIFORESET	(1 << 2)
#define	NS8250_FCR_DMAMODE	(1 << 3)
#define	NS8250_FCR_RXTRIGGERLOW	(1 << 6)
#define	NS8250_FCR_RXTRIGGERHI	(1 << 7)

/** Line Control Register definition. */
#define	NS8250_LCR_STOPBITS	(1 << 2)
#define	NS8250_LCR_PARITY	(1 << 3)
#define	NS8250_LCR_SENDBREAK	(1 << 6)
#define	NS8250_LCR_DLAB		(1 << 7)

/** Modem Control Register definition. */
#define	NS8250_MCR_DTR		(1 << 0)
#define	NS8250_MCR_RTS		(1 << 1)
#define	NS8250_MCR_OUT1		(1 << 2)
#define	NS8250_MCR_OUT2		(1 << 3)
#define	NS8250_MCR_LOOPBACK	(1 << 4)
#define	NS8250_MCR_ALL		(0x1f)

/** Line Status Register definition. */
#define	NS8250_LSR_RXREADY	(1 << 0)
#define	NS8250_LSR_OE		(1 << 1)
#define	NS8250_LSR_PE		(1 << 2)
#define	NS8250_LSR_FE		(1 << 3)
#define	NS8250_LSR_BREAK	(1 << 4)
#define	NS8250_LSR_THRE		(1 << 5)
#define	NS8250_LSR_TSE		(1 << 6)

/** Modem Status Register definition. */
#define	NS8250_MSR_DELTACTS	(1 << 0)
#define	NS8250_MSR_DELTADSR	(1 << 1)
#define	NS8250_MSR_RITRAILING	(1 << 2)
#define	NS8250_MSR_DELTADCD	(1 << 3)
#define	NS8250_MSR_CTS		(1 << 4)
#define	NS8250_MSR_DSR		(1 << 5)
#define	NS8250_MSR_RI		(1 << 6)
#define	NS8250_MSR_DCD		(1 << 7)
#define	NS8250_MSR_SIGNALS	(NS8250_MSR_CTS | NS8250_MSR_DSR \
    | NS8250_MSR_RI | NS8250_MSR_DCD)

/** The number of bits of one data unit send by the serial port. */
typedef enum {
	WORD_LENGTH_5,
	WORD_LENGTH_6,
	WORD_LENGTH_7,
	WORD_LENGTH_8
} word_length_t;

/** The number of stop bits used by the serial port. */
typedef enum {
	/** Use one stop bit. */
	ONE_STOP_BIT,
	/** 1.5 stop bits for word length 5, 2 stop bits otherwise. */
	TWO_STOP_BITS
} stop_bit_t;

/** 8250 UART registers layout. */
typedef struct {
	ioport8_t data;		/**< Data register. */
	ioport8_t ier;		/**< Interrupt Enable Reg. */
	ioport8_t iid;		/**< Interrupt ID Reg. */
	ioport8_t lcr;		/**< Line Control Reg. */
	ioport8_t mcr;		/**< Modem Control Reg. */
	ioport8_t lsr;		/**< Line Status Reg. */
	ioport8_t msr;		/**< Modem Status Reg. */
} ns8250_regs_t;

/** The driver data for the serial port devices. */
typedef struct ns8250 {
	/** DDF device node */
	ddf_dev_t *dev;
	/** DDF function node */
	ddf_fun_t *fun;
	/** Character device service */
	chardev_srvs_t cds;
	/** Parent session */
	async_sess_t *parent_sess;
	/** I/O registers **/
	ns8250_regs_t *regs;
	/** Are there any clients connected to the device? */
	unsigned client_connections;
	/** The irq assigned to this device. */
	int irq;
	/** IRQ capability handle */
	int irq_cap;
	/** The base i/o address of the devices registers. */
	uintptr_t io_addr;
	/** The i/o port used to access the serial ports registers. */
	ioport8_t *port;
	/** The buffer for incoming data. */
	cyclic_buffer_t input_buffer;
	/** The fibril mutex for synchronizing the access to the device. */
	fibril_mutex_t mutex;
	/** Indicates that some data has become available */
	fibril_condvar_t input_buffer_available;
	/** True if device is removed. */
	bool removed;
} ns8250_t;

/** Obtain soft-state structure from device node */
static ns8250_t *dev_ns8250(ddf_dev_t *dev)
{
	return ddf_dev_data_get(dev);
}

/** Obtain soft-state structure from function node */
static ns8250_t *fun_ns8250(ddf_fun_t *fun)
{
	return dev_ns8250(ddf_fun_get_dev(fun));
}

/** Obtain soft-state structure from chardev srv */
static ns8250_t *srv_ns8250(chardev_srv_t *srv)
{
	return (ns8250_t *)srv->srvs->sarg;
}


/** Find out if there is some incoming data available on the serial port.
 *
 * @param port		The base address of the serial port device's ports.
 * @return		True if there are data waiting to be read, false
 *			otherwise.
 */
static bool ns8250_received(ns8250_regs_t *regs)
{
	return (pio_read_8(&regs->lsr) & NS8250_LSR_RXREADY) != 0;
}

/** Read one byte from the serial port.
 *
 * @param port		The base address of the serial port device's ports.
 * @return		The data read.
 */
static uint8_t ns8250_read_8(ns8250_regs_t *regs)
{
	return pio_read_8(&regs->data);
}

/** Find out wheter it is possible to send data.
 *
 * @param port		The base address of the serial port device's ports.
 */
static bool is_transmit_empty(ns8250_regs_t *regs)
{
	return (pio_read_8(&regs->lsr) & NS8250_LSR_THRE) != 0;
}

/** Write one character on the serial port.
 *
 * @param port		The base address of the serial port device's ports.
 * @param c		The character to be written to the serial port device.
 */
static void ns8250_write_8(ns8250_regs_t *regs, uint8_t c)
{
	while (!is_transmit_empty(regs))
		;

	pio_write_8(&regs->data, c);
}

/** Read data from the serial port device.
 *
 * @param srv		Server-side connection data
 * @param buf		The output buffer for read data.
 * @param count		The number of bytes to be read.
 * @param nread		Place to store number of bytes actually read
 *
 * @return		EOK on success or non-zero error code
 */
static errno_t ns8250_read(chardev_srv_t *srv, void *buf, size_t count, size_t *nread)
{
	ns8250_t *ns = srv_ns8250(srv);
	char *bp = (char *) buf;
	size_t pos = 0;

	if (count == 0) {
		*nread = 0;
		return EOK;
	}

	fibril_mutex_lock(&ns->mutex);
	while (buf_is_empty(&ns->input_buffer))
		fibril_condvar_wait(&ns->input_buffer_available, &ns->mutex);
	while (!buf_is_empty(&ns->input_buffer) && pos < count) {
		bp[pos] = (char)buf_pop_front(&ns->input_buffer);
		pos++;
	}
	fibril_mutex_unlock(&ns->mutex);

	*nread = pos;
	return EOK;
}

/** Write a character to the serial port.
 *
 * @param ns		Serial port device
 * @param c		The character to be written
 */
static inline void ns8250_putchar(ns8250_t *ns, uint8_t c)
{
	fibril_mutex_lock(&ns->mutex);
	ns8250_write_8(ns->regs, c);
	fibril_mutex_unlock(&ns->mutex);
}

/** Write data to the serial port.
 *
 * @param srv		Server-side connection data
 * @param buf		The data to be written
 * @param count		The number of bytes to be written
 * @param nwritten	Place to store number of bytes successfully written
 * @return		EOK on success or non-zero error code
 */
static errno_t ns8250_write(chardev_srv_t *srv, const void *buf, size_t count,
    size_t *nwritten)
{
	ns8250_t *ns = srv_ns8250(srv);
	size_t idx;
	uint8_t *bp = (uint8_t *) buf;

	for (idx = 0; idx < count; idx++)
		ns8250_putchar(ns, bp[idx]);

	*nwritten = count;
	return EOK;
}

static errno_t ns8250_open(chardev_srvs_t *, chardev_srv_t *);
static errno_t ns8250_close(chardev_srv_t *);
static void ns8250_default_handler(chardev_srv_t *, ipc_callid_t, ipc_call_t *);

/** The character interface's callbacks. */
static chardev_ops_t ns8250_chardev_ops = {
	.open = ns8250_open,
	.close = ns8250_close,
	.read = ns8250_read,
	.write = ns8250_write,
	.def_handler = ns8250_default_handler
};

static void ns8250_char_conn(ipc_callid_t, ipc_call_t *, void *);

static errno_t ns8250_dev_add(ddf_dev_t *dev);
static errno_t ns8250_dev_remove(ddf_dev_t *dev);

/** The serial port device driver's standard operations. */
static driver_ops_t ns8250_ops = {
	.dev_add = &ns8250_dev_add,
	.dev_remove = &ns8250_dev_remove
};

/** The serial port device driver structure. */
static driver_t ns8250_driver = {
	.name = NAME,
	.driver_ops = &ns8250_ops
};

/** Clean up the serial port soft-state
 *
 * @param ns		Serial port device
 */
static void ns8250_dev_cleanup(ns8250_t *ns)
{
}

/** Enable the i/o ports of the device.
 *
 * @param ns		Serial port device
 * @return		True on success, false otherwise
 */
static bool ns8250_pio_enable(ns8250_t *ns)
{
	ddf_msg(LVL_DEBUG, "ns8250_pio_enable %s", ddf_dev_get_name(ns->dev));

	/* Gain control over port's registers. */
	if (pio_enable((void *) ns->io_addr, REG_COUNT,
	    (void **) &ns->port)) {
		ddf_msg(LVL_ERROR, "Cannot map the port %#" PRIxn
		    " for device %s.", ns->io_addr, ddf_dev_get_name(ns->dev));
		return false;
	}

	ns->regs = (ns8250_regs_t *)ns->port;

	return true;
}

/** Probe the serial port device for its presence.
 *
 * @param ns		Serial port device
 * @return		True if the device is present, false otherwise
 */
static bool ns8250_dev_probe(ns8250_t *ns)
{
	ddf_msg(LVL_DEBUG, "ns8250_dev_probe %s", ddf_dev_get_name(ns->dev));

	bool res = true;
	uint8_t olddata;

	olddata = pio_read_8(&ns->regs->mcr);

	pio_write_8(&ns->regs->mcr, NS8250_MCR_LOOPBACK);
	if (pio_read_8(&ns->regs->msr) & NS8250_MSR_SIGNALS)
		res = false;

	pio_write_8(&ns->regs->mcr, NS8250_MCR_ALL);
	if ((pio_read_8(&ns->regs->msr) & NS8250_MSR_SIGNALS)
	    != NS8250_MSR_SIGNALS)
		res = false;

	pio_write_8(&ns->regs->mcr, olddata);

	if (!res) {
		ddf_msg(LVL_DEBUG, "Device %s is not present.",
		    ddf_dev_get_name(ns->dev));
	}

	return res;
}

/** Initialize serial port device.
 *
 * @param ns		Serial port device
 * @return		Zero on success, error number otherwise
 */
static errno_t ns8250_dev_initialize(ns8250_t *ns)
{
	errno_t ret = EOK;

	ddf_msg(LVL_DEBUG, "ns8250_dev_initialize %s", ddf_dev_get_name(ns->dev));

	hw_resource_list_t hw_resources;
	memset(&hw_resources, 0, sizeof(hw_resource_list_t));

	/* Get hw resources. */
	ret = hw_res_get_resource_list(ns->parent_sess, &hw_resources);
	if (ret != EOK) {
		ddf_msg(LVL_ERROR, "Failed to get HW resources for device "
		    "%s.", ddf_dev_get_name(ns->dev));
		goto failed;
	}

	size_t i;
	hw_resource_t *res;
	bool irq = false;
	bool ioport = false;

	for (i = 0; i < hw_resources.count; i++) {
		res = &hw_resources.resources[i];
		switch (res->type) {
		case INTERRUPT:
			ns->irq = res->res.interrupt.irq;
			irq = true;
			ddf_msg(LVL_NOTE, "Device %s was assigned irq = 0x%x.",
			    ddf_dev_get_name(ns->dev), ns->irq);
			break;

		case IO_RANGE:
			ns->io_addr = res->res.io_range.address;
			if (res->res.io_range.size < REG_COUNT) {
				ddf_msg(LVL_ERROR, "I/O range assigned to "
				    "device %s is too small.", ddf_dev_get_name(ns->dev));
				ret = ELIMIT;
				goto failed;
			}
			ioport = true;
			ddf_msg(LVL_NOTE, "Device %s was assigned I/O address = "
			    "0x%#" PRIxn ".", ddf_dev_get_name(ns->dev), ns->io_addr);
    			break;

		default:
			break;
		}
	}

	if (!irq || !ioport) {
		ddf_msg(LVL_ERROR, "Missing HW resource(s) for device %s.",
		    ddf_dev_get_name(ns->dev));
		ret = ENOENT;
		goto failed;
	}

	hw_res_clean_resource_list(&hw_resources);
	return ret;

failed:
	ns8250_dev_cleanup(ns);
	hw_res_clean_resource_list(&hw_resources);
	return ret;
}

/** Enable interrupts on the serial port device.
 *
 * Interrupt when data is received
 *
 * @param port		The base address of the serial port device's ports.
 */
static inline void ns8250_port_interrupts_enable(ns8250_regs_t *regs)
{
	/* Interrupt when data received. */
	pio_write_8(&regs->ier, NS8250_IER_RXREADY | NS8250_IER_RXSTATUS);
	pio_write_8(&regs->mcr, NS8250_MCR_DTR | NS8250_MCR_RTS
	    | NS8250_MCR_OUT2);
}

/** Disable interrupts on the serial port device.
 *
 * @param port		The base address of the serial port device's ports
 */
static inline void ns8250_port_interrupts_disable(ns8250_regs_t *regs)
{
	pio_write_8(&regs->ier, 0x0);	/* Disable all interrupts. */
}

/** Enable interrupts for the serial port device.
 *
 * @param ns		Serial port device
 * @return		Zero on success, error number otherwise
 */
static errno_t ns8250_interrupt_enable(ns8250_t *ns)
{
	/* Enable interrupt using IRC service. */
	errno_t rc = hw_res_enable_interrupt(ns->parent_sess, ns->irq);
	if (rc != EOK)
		return EIO;

	/* Read LSR to clear possible previous LSR interrupt */
	pio_read_8(&ns->regs->lsr);

	/* Enable interrupt on the serial port. */
	ns8250_port_interrupts_enable(ns->regs);

	return EOK;
}

/** Set Divisor Latch Access Bit.
 *
 * When the Divisor Latch Access Bit is set, it is possible to set baud rate of
 * the serial port device.
 *
 * @param port		The base address of the serial port device's ports.
 */
static inline void enable_dlab(ns8250_regs_t *regs)
{
	uint8_t val = pio_read_8(&regs->lcr);
	pio_write_8(&regs->lcr, val | NS8250_LCR_DLAB);
}

/** Clear Divisor Latch Access Bit.
 *
 * @param port		The base address of the serial port device's ports.
 */
static inline void clear_dlab(ns8250_regs_t *regs)
{
	uint8_t val = pio_read_8(&regs->lcr);
	pio_write_8(&regs->lcr, val & (~NS8250_LCR_DLAB));
}

/** Set baud rate of the serial communication on the serial device.
 *
 * @param port		The base address of the serial port device's ports.
 * @param baud_rate	The baud rate to be used by the device.
 * @return		Zero on success, error number otherwise (EINVAL
 *			if the specified baud_rate is not valid).
 */
static errno_t ns8250_port_set_baud_rate(ns8250_regs_t *regs, unsigned int baud_rate)
{
	uint16_t divisor;
	uint8_t div_low, div_high;

	if (baud_rate < 50 || MAX_BAUD_RATE % baud_rate != 0) {
		ddf_msg(LVL_ERROR, "Invalid baud rate %d requested.",
		    baud_rate);
		return EINVAL;
	}

	divisor = MAX_BAUD_RATE / baud_rate;
	div_low = (uint8_t)divisor;
	div_high = (uint8_t)(divisor >> 8);

	/* Enable DLAB to be able to access baud rate divisor. */
	enable_dlab(regs);

	/* Set divisor low byte. */
	pio_write_8(&regs->data, div_low);
	/* Set divisor high byte. */
	pio_write_8(&regs->ier, div_high);

	clear_dlab(regs);

	return EOK;
}

/** Get baud rate used by the serial port device.
 *
 * @param port		The base address of the serial port device's ports.
 * @param baud_rate	The ouput parameter to which the baud rate is stored.
 */
static unsigned int ns8250_port_get_baud_rate(ns8250_regs_t *regs)
{
	uint16_t divisor;
	uint8_t div_low, div_high;

	/* Enable DLAB to be able to access baud rate divisor. */
	enable_dlab(regs);

	/* Get divisor low byte. */
	div_low = pio_read_8(&regs->data);
	/* Get divisor high byte. */
	div_high = pio_read_8(&regs->ier);

	clear_dlab(regs);

	divisor = (div_high << 8) | div_low;
	return MAX_BAUD_RATE / divisor;
}

/** Get the parameters of the serial communication set on the serial port
 * device.
 *
 * @param parity	The parity used.
 * @param word_length	The length of one data unit in bits.
 * @param stop_bits	The number of stop bits used (one or two).
 */
static void ns8250_port_get_com_props(ns8250_regs_t *regs, unsigned int *parity,
    unsigned int *word_length, unsigned int *stop_bits)
{
	uint8_t val;

	val = pio_read_8(&regs->lcr);
	*parity = ((val >> NS8250_LCR_PARITY) & 7);

	/* Silence warnings */
	*word_length = 0;

	switch (val & 3) {
	case WORD_LENGTH_5:
		*word_length = 5;
		break;
	case WORD_LENGTH_6:
		*word_length = 6;
		break;
	case WORD_LENGTH_7:
		*word_length = 7;
		break;
	case WORD_LENGTH_8:
		*word_length = 8;
		break;
	}

	if ((val >> NS8250_LCR_STOPBITS) & 1)
		*stop_bits = 2;
	else
		*stop_bits = 1;
}

/** Set the parameters of the serial communication on the serial port device.
 *
 * @param parity	The parity to be used.
 * @param word_length	The length of one data unit in bits.
 * @param stop_bits	The number of stop bits used (one or two).
 * @return		Zero on success, EINVAL if some of the specified values
 *			is invalid.
 */
static errno_t ns8250_port_set_com_props(ns8250_regs_t *regs, unsigned int parity,
    unsigned int word_length, unsigned int stop_bits)
{
	uint8_t val;

	switch (word_length) {
	case 5:
		val = WORD_LENGTH_5;
		break;
	case 6:
		val = WORD_LENGTH_6;
		break;
	case 7:
		val = WORD_LENGTH_7;
		break;
	case 8:
		val = WORD_LENGTH_8;
		break;
	default:
		return EINVAL;
	}

	switch (stop_bits) {
	case 1:
		val |= ONE_STOP_BIT << NS8250_LCR_STOPBITS;
		break;
	case 2:
		val |= TWO_STOP_BITS << NS8250_LCR_STOPBITS;
		break;
	default:
		return EINVAL;
	}

	switch (parity) {
	case SERIAL_NO_PARITY:
	case SERIAL_ODD_PARITY:
	case SERIAL_EVEN_PARITY:
	case SERIAL_MARK_PARITY:
	case SERIAL_SPACE_PARITY:
		val |= parity << NS8250_LCR_PARITY;
		break;
	default:
		return EINVAL;
	}

	pio_write_8(&regs->lcr, val);

	return EOK;
}

/** Initialize the serial port device.
 *
 * Set the default parameters of the serial communication.
 *
 * @param ns		Serial port device
 */
static void ns8250_initialize_port(ns8250_t *ns)
{
	/* Disable interrupts. */
	ns8250_port_interrupts_disable(ns->regs);
	/* Set baud rate. */
	ns8250_port_set_baud_rate(ns->regs, 38400);
	/* 8 bits, no parity, two stop bits. */
	ns8250_port_set_com_props(ns->regs, SERIAL_NO_PARITY, 8, 2);
	/*
	 * Enable FIFO, clear them, with 4-byte threshold for greater
	 * reliability.
	 */
	pio_write_8(&ns->regs->iid, NS8250_FCR_FIFOENABLE
	    | NS8250_FCR_RXFIFORESET | NS8250_FCR_TXFIFORESET
	    | NS8250_FCR_RXTRIGGERLOW);
	/*
	 * RTS/DSR set (Request to Send and Data Terminal Ready lines enabled),
	 * Aux Output2 set - needed for interrupts.
	 */
	pio_write_8(&ns->regs->mcr, NS8250_MCR_DTR | NS8250_MCR_RTS
	    | NS8250_MCR_OUT2);
}

/** Deinitialize the serial port device.
 *
 * @param ns		Serial port device
 */
static void ns8250_port_cleanup(ns8250_t *ns)
{
	/* Disable FIFO */
	pio_write_8(&ns->regs->iid, 0x00);
	/* Disable DTR, RTS, OUT1, OUT2 (int. enable) */
	pio_write_8(&ns->regs->mcr, 0x00);
	/* Disable all interrupts from the port */
	ns8250_port_interrupts_disable(ns->regs);
}

/** Read the data from the serial port device and store them to the input
 * buffer.
 *
 * @param ns		Serial port device
 */
static void ns8250_read_from_device(ns8250_t *ns)
{
	ns8250_regs_t *regs = ns->regs;
	bool cont = true;

	fibril_mutex_lock(&ns->mutex);
	while (cont) {
		cont = ns8250_received(regs);
		if (cont) {
			uint8_t val = ns8250_read_8(regs);

			if (ns->client_connections > 0) {
				bool buf_was_empty = buf_is_empty(&ns->input_buffer);
				if (!buf_push_back(&ns->input_buffer, val)) {
					ddf_msg(LVL_WARN, "Buffer overflow on "
					    "%s.", ddf_dev_get_name(ns->dev));
					break;
				} else {
					ddf_msg(LVL_DEBUG2, "Character %c saved "
					    "to the buffer of %s.",
					    val, ddf_dev_get_name(ns->dev));
					if (buf_was_empty)
						fibril_condvar_broadcast(&ns->input_buffer_available);
				}
			}
		}
	}
	fibril_mutex_unlock(&ns->mutex);
	fibril_yield();
}

/** The interrupt handler.
 *
 * The serial port is initialized to interrupt when some data come or line
 * status register changes, so the interrupt is handled by reading the incoming
 * data and reading the line status register.
 *
 * @param dev The serial port device.
 *
 */
static inline void ns8250_interrupt_handler(ipc_call_t *icall, ddf_dev_t *dev)
{
	ns8250_t *ns = dev_ns8250(dev);
	uint8_t iir = pio_read_8(&ns->regs->iid);
	if ((iir & NS8250_IID_CAUSE_MASK) == NS8250_IID_CAUSE_RXSTATUS) {
		uint8_t lsr = pio_read_8(&ns->regs->lsr);
		if (lsr & NS8250_LSR_OE) {
			ddf_msg(LVL_WARN, "Overrun error on %s", ddf_dev_get_name(ns->dev));
		}
	}

	ns8250_read_from_device(ns);
	hw_res_clear_interrupt(ns->parent_sess, ns->irq);
}

/** Register the interrupt handler for the device.
 *
 * @param ns		Serial port device
 */
static inline errno_t ns8250_register_interrupt_handler(ns8250_t *ns,
    cap_handle_t *handle)
{
	return register_interrupt_handler(ns->dev, ns->irq,
	    ns8250_interrupt_handler, NULL, handle);
}

/** Unregister the interrupt handler for the device.
 *
 * @param ns		Serial port device
 */
static inline errno_t ns8250_unregister_interrupt_handler(ns8250_t *ns)
{
	return unregister_interrupt_handler(ns->dev, ns->irq_cap);
}

/** The dev_add callback method of the serial port driver.
 *
 * Probe and initialize the newly added device.
 *
 * @param dev		The serial port device.
 */
static errno_t ns8250_dev_add(ddf_dev_t *dev)
{
	ns8250_t *ns = NULL;
	ddf_fun_t *fun = NULL;
	bool need_cleanup = false;
	bool need_unreg_intr_handler = false;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "ns8250_dev_add %s (handle = %d)",
	    ddf_dev_get_name(dev), (int) ddf_dev_get_handle(dev));

	/* Allocate soft-state for the device */
	ns = ddf_dev_data_alloc(dev, sizeof(ns8250_t));
	if (ns == NULL) {
		rc = ENOMEM;
		goto fail;
	}

	fibril_mutex_initialize(&ns->mutex);
	fibril_condvar_initialize(&ns->input_buffer_available);
	ns->dev = dev;

	ns->parent_sess = ddf_dev_parent_sess_get(ns->dev);
	if (ns->parent_sess == NULL) {
		ddf_msg(LVL_ERROR, "Failed to connect to parent driver of "
		    "device %s.", ddf_dev_get_name(ns->dev));
		rc = EIO;
		goto fail;
	}

	rc = ns8250_dev_initialize(ns);
	if (rc != EOK)
		goto fail;

	need_cleanup = true;

	if (!ns8250_pio_enable(ns)) {
		rc = EADDRNOTAVAIL;
		goto fail;
	}

	/* Find out whether the device is present. */
	if (!ns8250_dev_probe(ns)) {
		rc = ENOENT;
		goto fail;
	}

	/* Serial port initialization (baud rate etc.). */
	ns8250_initialize_port(ns);

	/* Register interrupt handler. */
	rc = ns8250_register_interrupt_handler(ns, &ns->irq_cap);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to register interrupt handler.");
		rc = EADDRNOTAVAIL;
		goto fail;
	}
	need_unreg_intr_handler = true;

	/* Enable interrupt. */
	rc = ns8250_interrupt_enable(ns);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to enable the interrupt. Error code = "
		    "%s.", str_error_name(rc));
		goto fail;
	}

	fun = ddf_fun_create(dev, fun_exposed, "a");
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function.");
		goto fail;
	}

	ddf_fun_set_conn_handler(fun, ns8250_char_conn);

	chardev_srvs_init(&ns->cds);
	ns->cds.ops = &ns8250_chardev_ops;
	ns->cds.sarg = ns;

	rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function.");
		goto fail;
	}

	ns->fun = fun;

	ddf_fun_add_to_category(fun, "serial");

	ddf_msg(LVL_NOTE, "Device %s successfully initialized.",
	    ddf_dev_get_name(dev));

	return EOK;
fail:
	if (fun != NULL)
		ddf_fun_destroy(fun);
	if (need_unreg_intr_handler)
		ns8250_unregister_interrupt_handler(ns);
	if (need_cleanup)
		ns8250_dev_cleanup(ns);
	return rc;
}

static errno_t ns8250_dev_remove(ddf_dev_t *dev)
{
	ns8250_t *ns = dev_ns8250(dev);
	errno_t rc;

	fibril_mutex_lock(&ns->mutex);
	if (ns->client_connections > 0) {
		fibril_mutex_unlock(&ns->mutex);
		return EBUSY;
	}
	ns->removed = true;
	fibril_mutex_unlock(&ns->mutex);

	rc = ddf_fun_unbind(ns->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to unbind function.");
		return rc;
	}

	ddf_fun_destroy(ns->fun);

	ns8250_port_cleanup(ns);
	ns8250_unregister_interrupt_handler(ns);
	ns8250_dev_cleanup(ns);
	return EOK;
}

/** Open the device.
 *
 * This is a callback function called when a client tries to connect to the
 * device.
 *
 * @param srvs		Service structure
 * @param srv		Server-side connection structure
 */
static errno_t ns8250_open(chardev_srvs_t *srvs, chardev_srv_t *srv)
{
	ns8250_t *ns = srv_ns8250(srv);
	errno_t res;

	fibril_mutex_lock(&ns->mutex);
	if (ns->removed) {
		res = ENXIO;
	} else {
		res = EOK;
		ns->client_connections++;
	}
	fibril_mutex_unlock(&ns->mutex);

	return res;
}

/** Close the device.
 *
 * This is a callback function called when a client tries to disconnect from
 * the device.
 *
 * @param srv		Server-side connection structure
 */
static errno_t ns8250_close(chardev_srv_t *srv)
{
	ns8250_t *data = srv_ns8250(srv);

	fibril_mutex_lock(&data->mutex);

	assert(data->client_connections > 0);

	if (!(--data->client_connections))
		buf_clear(&data->input_buffer);

	fibril_mutex_unlock(&data->mutex);

	return EOK;
}

/** Get parameters of the serial communication which are set to the specified
 * device.
 *
 * @param dev		The serial port device.
 * @param baud_rate	The baud rate used by the device.
 * @param parity	The type of parity used by the device.
 * @param word_length	The size of one data unit in bits.
 * @param stop_bits	The number of stop bits used.
 */
static void
ns8250_get_props(ddf_dev_t *dev, unsigned int *baud_rate, unsigned int *parity,
    unsigned int *word_length, unsigned int* stop_bits)
{
	ns8250_t *data = dev_ns8250(dev);
	ns8250_regs_t *regs = data->regs;

	fibril_mutex_lock(&data->mutex);
	ns8250_port_interrupts_disable(regs);
	*baud_rate = ns8250_port_get_baud_rate(regs);
	ns8250_port_get_com_props(regs, parity, word_length, stop_bits);
	ns8250_port_interrupts_enable(regs);
	fibril_mutex_unlock(&data->mutex);

	ddf_msg(LVL_DEBUG, "ns8250_get_props: baud rate %d, parity 0x%x, word "
	    "length %d, stop bits %d", *baud_rate, *parity, *word_length,
	    *stop_bits);
}

/** Set parameters of the serial communication to the specified serial port
 * device.
 *
 * @param dev		The serial port device.
 * @param baud_rate	The baud rate to be used by the device.
 * @param parity	The type of parity to be used by the device.
 * @param word_length	The size of one data unit in bits.
 * @param stop_bits	The number of stop bits to be used.
 */
static errno_t ns8250_set_props(ddf_dev_t *dev, unsigned int baud_rate,
    unsigned int parity, unsigned int word_length, unsigned int stop_bits)
{
	ddf_msg(LVL_DEBUG, "ns8250_set_props: baud rate %d, parity 0x%x, word "
	    "length %d, stop bits %d", baud_rate, parity, word_length,
	    stop_bits);

	ns8250_t *data = dev_ns8250(dev);
	ns8250_regs_t *regs = data->regs;
	errno_t ret;

	fibril_mutex_lock(&data->mutex);
	ns8250_port_interrupts_disable(regs);
	ret = ns8250_port_set_baud_rate(regs, baud_rate);
	if (ret == EOK)
		ret = ns8250_port_set_com_props(regs, parity, word_length, stop_bits);
	ns8250_port_interrupts_enable(regs);
	fibril_mutex_unlock(&data->mutex);

	return ret;
}

/** Default handler for client requests which are not handled by the standard
 * interfaces.
 *
 * Configure the parameters of the serial communication.
 */
static void ns8250_default_handler(chardev_srv_t *srv, ipc_callid_t callid,
    ipc_call_t *call)
{
	ns8250_t *ns8250 = srv_ns8250(srv);
	sysarg_t method = IPC_GET_IMETHOD(*call);
	errno_t ret;
	unsigned int baud_rate, parity, word_length, stop_bits;

	switch (method) {
	case SERIAL_GET_COM_PROPS:
		ns8250_get_props(ns8250->dev, &baud_rate, &parity, &word_length,
		    &stop_bits);
		async_answer_4(callid, EOK, baud_rate, parity, word_length,
		    stop_bits);
		break;

	case SERIAL_SET_COM_PROPS:
 		baud_rate = IPC_GET_ARG1(*call);
		parity = IPC_GET_ARG2(*call);
		word_length = IPC_GET_ARG3(*call);
		stop_bits = IPC_GET_ARG4(*call);
		ret = ns8250_set_props(ns8250->dev, baud_rate, parity, word_length,
		    stop_bits);
		async_answer_0(callid, ret);
		break;

	default:
		async_answer_0(callid, ENOTSUP);
	}
}

void ns8250_char_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	ns8250_t *ns8250 = fun_ns8250((ddf_fun_t *)arg);

	chardev_conn(iid, icall, &ns8250->cds);
}

/** Initialize the serial port driver.
 *
 * Initialize device operations structures with callback methods for handling
 * client requests to the serial port devices.
 */
static void ns8250_init(void)
{
	ddf_log_init(NAME);
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS serial port driver\n");
	ns8250_init();
	return ddf_driver_main(&ns8250_driver);
}

/**
 * @}
 */
