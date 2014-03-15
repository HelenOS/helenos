/*
 * Copyright (c) 2010 Lenka Trochtova
 * Copyright (c) 2011 Jiri Svoboda
 * Copyright (c) 2013 Jakub Klama
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
 * @defgroup grlib_uart Serial port driver.
 * @brief HelenOS serial port driver.
 * @{
 */

/** @file
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <str.h>
#include <ctype.h>
#include <macros.h>
#include <malloc.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ddi.h>

#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <ddf/log.h>
#include <ops/char_dev.h>

#include <ns.h>
#include <ipc/services.h>
#include <ipc/irc.h>
#include <device/hw_res.h>
#include <ipc/serial_ctl.h>

#include "cyclic_buffer.h"

#define NAME "grlib_uart"

#define REG_COUNT 5
#define MAX_BAUD_RATE 115200

#define LVL_DEBUG LVL_ERROR

#define GRLIB_UART_STATUS_DR  (1 << 0)
#define GRLIB_UART_STATUS_TS  (1 << 1)
#define GRLIB_UART_STATUS_TE  (1 << 2)
#define GRLIB_UART_STATUS_BR  (1 << 3)
#define GRLIB_UART_STATUS_OV  (1 << 4)
#define GRLIB_UART_STATUS_PE  (1 << 5)
#define GRLIB_UART_STATUS_FE  (1 << 6)
#define GRLIB_UART_STATUS_TH  (1 << 7)
#define GRLIB_UART_STATUS_RH  (1 << 8)
#define GRLIB_UART_STATUS_TF  (1 << 9)
#define GRLIB_UART_STATUS_RF  (1 << 10)

#define GRLIB_UART_CONTROL_RE  (1 << 0)
#define GRLIB_UART_CONTROL_TE  (1 << 1)
#define GRLIB_UART_CONTROL_RI  (1 << 2)
#define GRLIB_UART_CONTROL_TI  (1 << 3)
#define GRLIB_UART_CONTROL_PS  (1 << 4)
#define GRLIB_UART_CONTROL_PE  (1 << 5)
#define GRLIB_UART_CONTROL_FL  (1 << 6)
#define GRLIB_UART_CONTROL_LB  (1 << 7)
#define GRLIB_UART_CONTROL_EC  (1 << 8)
#define GRLIB_UART_CONTROL_TF  (1 << 9)
#define GRLIB_UART_CONTROL_RF  (1 << 10)
#define GRLIB_UART_CONTROL_DB  (1 << 11)
#define GRLIB_UART_CONTROL_BI  (1 << 12)
#define GRLIB_UART_CONTROL_DI  (1 << 13)
#define GRLIB_UART_CONTROL_SI  (1 << 14)
#define GRLIB_UART_CONTROL_FA  (1 << 31)

typedef struct {
	unsigned int fa : 1;
	unsigned int : 16;
	unsigned int si : 1;
	unsigned int di : 1;
	unsigned int bi : 1;
	unsigned int db : 1;
	unsigned int rf : 1;
	unsigned int tf : 1;
	unsigned int ec : 1;
	unsigned int lb : 1;
	unsigned int fl : 1;
	unsigned int pe : 1;
	unsigned int ps : 1;
	unsigned int ti : 1;
	unsigned int ri : 1;
	unsigned int te : 1;
	unsigned int re : 1;
} grlib_uart_control_t;

/** GRLIB UART registers */
typedef struct {
	ioport32_t data;
	ioport32_t status;
	ioport32_t control;
	ioport32_t scaler;
	ioport32_t debug;
} grlib_uart_regs_t;

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

/** The driver data for the serial port devices. */
typedef struct grlib_uart {
	/** DDF device node */
	ddf_dev_t *dev;
	/** DDF function node */
	ddf_fun_t *fun;
	/** I/O registers **/
	grlib_uart_regs_t *regs;
	/** Are there any clients connected to the device? */
	unsigned client_connections;
	/** The irq assigned to this device. */
	int irq;
	/** The base i/o address of the devices registers. */
	uintptr_t regs_addr;
	/** The buffer for incoming data. */
	cyclic_buffer_t input_buffer;
	/** The fibril mutex for synchronizing the access to the device. */
	fibril_mutex_t mutex;
	/** Indicates that some data has become available */
	fibril_condvar_t input_buffer_available;
	/** True if device is removed. */
	bool removed;
} grlib_uart_t;

/** Obtain soft-state structure from device node */
static grlib_uart_t *dev_grlib_uart(ddf_dev_t *dev)
{
	return ddf_dev_data_get(dev);
}

/** Obtain soft-state structure from function node */
static grlib_uart_t *fun_grlib_uart(ddf_fun_t *fun)
{
	return dev_grlib_uart(ddf_fun_get_dev(fun));
}

/** Find out if there is some incoming data available on the serial port.
 *
 * @param port The base address of the serial port device's ports.
 *
 * @return True if there are data waiting to be read.
 * @return False otherwise.
 *
 */
static bool grlib_uart_received(grlib_uart_regs_t *regs)
{
	return ((pio_read_32(&regs->status) & GRLIB_UART_STATUS_DR) != 0);
}

/** Read one byte from the serial port.
 *
 * @param port The base address of the serial port device's ports.
 *
 * @return The data read.
 *
 */
static uint8_t grlib_uart_read_8(grlib_uart_regs_t *regs)
{
	return (uint8_t) pio_read_32(&regs->data);
}

/** Find out wheter it is possible to send data.
 *
 * @param port The base address of the serial port device's ports.
 *
 */
static bool is_transmit_empty(grlib_uart_regs_t *regs)
{
	return ((pio_read_32(&regs->status) & GRLIB_UART_STATUS_TS) != 0);
}

/** Write one character on the serial port.
 *
 * @param port The base address of the serial port device's ports.
 * @param c    The character to be written to the serial port device.
 *
 */
static void grlib_uart_write_8(grlib_uart_regs_t *regs, uint8_t c)
{
	while (!is_transmit_empty(regs));
	
	pio_write_32(&regs->data, (uint32_t) c);
}

/** Read data from the serial port device.
 *
 * @param fun   The serial port function
 * @param buf   The output buffer for read data.
 * @param count The number of bytes to be read.
 *
 * @return The number of bytes actually read on success,
 * @return Negative error number otherwise.
 *
 */
static int grlib_uart_read(ddf_fun_t *fun, char *buf, size_t count)
{
	if (count == 0)
		return 0;
	
	grlib_uart_t *ns = fun_grlib_uart(fun);
	
	fibril_mutex_lock(&ns->mutex);
	
	while (buf_is_empty(&ns->input_buffer))
		fibril_condvar_wait(&ns->input_buffer_available, &ns->mutex);
	
	int ret = 0;
	while ((!buf_is_empty(&ns->input_buffer)) && ((size_t) ret < count)) {
		buf[ret] = (char) buf_pop_front(&ns->input_buffer);
		ret++;
	}
	
	fibril_mutex_unlock(&ns->mutex);
	
	return ret;
}

/** Write a character to the serial port.
 *
 * @param ns Serial port device
 * @param c  The character to be written
 *
 */
static inline void grlib_uart_putchar(grlib_uart_t *ns, uint8_t c)
{
	fibril_mutex_lock(&ns->mutex);
	grlib_uart_write_8(ns->regs, c);
	fibril_mutex_unlock(&ns->mutex);
}

/** Write data to the serial port.
 *
 * @param fun   The serial port function
 * @param buf   The data to be written
 * @param count The number of bytes to be written
 *
 * @return Zero on success
 *
 */
static int grlib_uart_write(ddf_fun_t *fun, char *buf, size_t count)
{
	grlib_uart_t *ns = fun_grlib_uart(fun);
	
	for (size_t idx = 0; idx < count; idx++)
		grlib_uart_putchar(ns, (uint8_t) buf[idx]);
	
	return count;
}

static ddf_dev_ops_t grlib_uart_dev_ops;

/** The character interface's callbacks. */
static char_dev_ops_t grlib_uart_char_dev_ops = {
	.read = &grlib_uart_read,
	.write = &grlib_uart_write
};

static int grlib_uart_dev_add(ddf_dev_t *);
static int grlib_uart_dev_remove(ddf_dev_t *);

/** The serial port device driver's standard operations. */
static driver_ops_t grlib_uart_ops = {
	.dev_add = &grlib_uart_dev_add,
	.dev_remove = &grlib_uart_dev_remove
};

/** The serial port device driver structure. */
static driver_t grlib_uart_driver = {
	.name = NAME,
	.driver_ops = &grlib_uart_ops
};

/** Clean up the serial port soft-state
 *
 * @param ns Serial port device
 *
 */
static void grlib_uart_dev_cleanup(grlib_uart_t *ns)
{
}

/** Enable the i/o ports of the device.
 *
 * @param ns Serial port device
 *
 * @return True on success, false otherwise
 *
 */
static bool grlib_uart_pio_enable(grlib_uart_t *ns)
{
	ddf_msg(LVL_DEBUG, "grlib_uart_pio_enable %s", ddf_dev_get_name(ns->dev));
	
	/* Gain control over port's registers. */
	if (pio_enable((void *) ns->regs_addr, REG_COUNT,
	    (void **) &ns->regs)) {
		ddf_msg(LVL_ERROR, "Cannot map the port %#" PRIx32
		    " for device %s.", ns->regs_addr, ddf_dev_get_name(ns->dev));
		return false;
	}
	
	return true;
}

/** Probe the serial port device for its presence.
 *
 * @param ns Serial port device
 *
 * @return True if the device is present, false otherwise
 *
 */
static bool grlib_uart_dev_probe(grlib_uart_t *ns)
{
	ddf_msg(LVL_DEBUG, "grlib_uart_dev_probe %s", ddf_dev_get_name(ns->dev));
	
	return true;
}

/** Initialize serial port device.
 *
 * @param ns Serial port device
 *
 * @return Zero on success, negative error number otherwise
 *
 */
static int grlib_uart_dev_initialize(grlib_uart_t *ns)
{
	ddf_msg(LVL_DEBUG, "grlib_uart_dev_initialize %s", ddf_dev_get_name(ns->dev));
	
	hw_resource_list_t hw_resources;
	memset(&hw_resources, 0, sizeof(hw_resource_list_t));
	
	int ret = EOK;
	
	/* Connect to the parent's driver. */
	async_sess_t *parent_sess = ddf_dev_parent_sess_create(ns->dev,
	    EXCHANGE_SERIALIZE);
	if (parent_sess == NULL) {
		ddf_msg(LVL_ERROR, "Failed to connect to parent driver of "
		    "device %s.", ddf_dev_get_name(ns->dev));
		ret = ENOENT;
		goto failed;
	}
	
	/* Get hw resources. */
	ret = hw_res_get_resource_list(parent_sess, &hw_resources);
	if (ret != EOK) {
		ddf_msg(LVL_ERROR, "Failed to get HW resources for device "
		    "%s.", ddf_dev_get_name(ns->dev));
		goto failed;
	}
	
	bool irq = false;
	bool ioport = false;
	
	for (size_t i = 0; i < hw_resources.count; i++) {
		hw_resource_t *res = &hw_resources.resources[i];
		switch (res->type) {
		case INTERRUPT:
			ns->irq = res->res.interrupt.irq;
			irq = true;
			ddf_msg(LVL_NOTE, "Device %s was assigned irq = 0x%x.",
			    ddf_dev_get_name(ns->dev), ns->irq);
			break;
			
		case MEM_RANGE:
			ns->regs_addr = res->res.mem_range.address;
			if (res->res.mem_range.size < REG_COUNT) {
				ddf_msg(LVL_ERROR, "I/O range assigned to "
				    "device %s is too small.", ddf_dev_get_name(ns->dev));
				ret = ELIMIT;
				goto failed;
			}
			ioport = true;
			ddf_msg(LVL_NOTE, "Device %s was assigned I/O address = "
			    "0x%x.", ddf_dev_get_name(ns->dev), ns->regs_addr);
    			break;
			
		default:
			break;
		}
	}
	
	if ((!irq) || (!ioport)) {
		ddf_msg(LVL_ERROR, "Missing HW resource(s) for device %s.",
		    ddf_dev_get_name(ns->dev));
		ret = ENOENT;
		goto failed;
	}
	
	hw_res_clean_resource_list(&hw_resources);
	return ret;
	
failed:
	grlib_uart_dev_cleanup(ns);
	hw_res_clean_resource_list(&hw_resources);
	return ret;
}

/** Enable interrupts on the serial port device.
 *
 * Interrupt when data is received
 *
 * @param port The base address of the serial port device's ports.
 *
 */
static inline void grlib_uart_port_interrupts_enable(grlib_uart_regs_t *regs)
{
	/* Interrupt when data received. */
	uint32_t control = pio_read_32(&regs->control);
	pio_write_32(&regs->control, control | GRLIB_UART_CONTROL_RE);
}

/** Disable interrupts on the serial port device.
 *
 * @param port The base address of the serial port device's ports.
 *
 */
static inline void grlib_uart_port_interrupts_disable(grlib_uart_regs_t *regs)
{
	uint32_t control = pio_read_32(&regs->control);
	pio_write_32(&regs->control, control & (~GRLIB_UART_CONTROL_RE));
}

/** Enable interrupts for the serial port device.
 *
 * @param ns Serial port device
 *
 * @return Zero on success, negative error number otherwise
 *
 */
static int grlib_uart_interrupt_enable(grlib_uart_t *ns)
{
	/* Enable interrupt on the serial port. */
	grlib_uart_port_interrupts_enable(ns->regs);
	
	return EOK;
}

static int grlib_uart_port_set_baud_rate(grlib_uart_regs_t *regs,
    unsigned int baud_rate)
{
	if ((baud_rate < 50) || (MAX_BAUD_RATE % baud_rate != 0)) {
		ddf_msg(LVL_ERROR, "Invalid baud rate %d requested.",
		    baud_rate);
		return EINVAL;
	}
	
	/* XXX: Set baud rate */
	
	return EOK;
}

/** Set the parameters of the serial communication on the serial port device.
 *
 * @param parity      The parity to be used.
 * @param word_length The length of one data unit in bits.
 * @param stop_bits   The number of stop bits used (one or two).
 *
 * @return Zero on success.
 * @return EINVAL if some of the specified values is invalid.
 *
 */
static int grlib_uart_port_set_com_props(grlib_uart_regs_t *regs,
    unsigned int parity, unsigned int word_length, unsigned int stop_bits)
{
	uint32_t val = pio_read_32(&regs->control);
	
	switch (parity) {
	case SERIAL_NO_PARITY:
	case SERIAL_ODD_PARITY:
	case SERIAL_EVEN_PARITY:
	case SERIAL_MARK_PARITY:
	case SERIAL_SPACE_PARITY:
		val |= GRLIB_UART_CONTROL_PE;
		break;
	default:
		return EINVAL;
	}
	
	pio_write_32(&regs->control, val);
	
	return EOK;
}

/** Initialize the serial port device.
 *
 * Set the default parameters of the serial communication.
 *
 * @param ns Serial port device
 *
 */
static void grlib_uart_initialize_port(grlib_uart_t *ns)
{
	/* Disable interrupts. */
	grlib_uart_port_interrupts_disable(ns->regs);
	
	/* Set baud rate. */
	grlib_uart_port_set_baud_rate(ns->regs, 38400);
	
	/* 8 bits, no parity, two stop bits. */
	grlib_uart_port_set_com_props(ns->regs, SERIAL_NO_PARITY, 8, 2);
	
	/*
	 * Enable FIFO, clear them, with 4-byte threshold for greater
	 * reliability.
	 */
	pio_write_32(&ns->regs->control, GRLIB_UART_CONTROL_RE |
	    GRLIB_UART_CONTROL_TE | GRLIB_UART_CONTROL_RF |
	    GRLIB_UART_CONTROL_TF | GRLIB_UART_CONTROL_RI |
	    GRLIB_UART_CONTROL_FA);
}

/** Deinitialize the serial port device.
 *
 * @param ns Serial port device
 *
 */
static void grlib_uart_port_cleanup(grlib_uart_t *ns)
{
	grlib_uart_port_interrupts_disable(ns->regs);
}

/** Read the data from the serial port device and store them to the input
 * buffer.
 *
 * @param ns Serial port device
 *
 */
static void grlib_uart_read_from_device(grlib_uart_t *ns)
{
	grlib_uart_regs_t *regs = ns->regs;
	bool cont = true;
	
	fibril_mutex_lock(&ns->mutex);
	
	while (cont) {
		cont = grlib_uart_received(regs);
		if (cont) {
			uint8_t val = grlib_uart_read_8(regs);
			
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
static inline void grlib_uart_interrupt_handler(ddf_dev_t *dev,
    ipc_callid_t iid, ipc_call_t *icall)
{
	grlib_uart_t *ns = dev_grlib_uart(dev);
	
	uint32_t status = pio_read_32(&ns->regs->status);
	
	if (status & GRLIB_UART_STATUS_RF) {
		if (status & GRLIB_UART_STATUS_OV)
			ddf_msg(LVL_WARN, "Overrun error on %s", ddf_dev_get_name(ns->dev));
	}
	
	grlib_uart_read_from_device(ns);
}

/** Register the interrupt handler for the device.
 *
 * @param ns Serial port device
 *
 */
static inline int grlib_uart_register_interrupt_handler(grlib_uart_t *ns)
{
	return register_interrupt_handler(ns->dev, ns->irq,
	    grlib_uart_interrupt_handler, NULL);
}

/** Unregister the interrupt handler for the device.
 *
 * @param ns Serial port device
 *
 */
static inline int grlib_uart_unregister_interrupt_handler(grlib_uart_t *ns)
{
	return unregister_interrupt_handler(ns->dev, ns->irq);
}

/** The dev_add callback method of the serial port driver.
 *
 * Probe and initialize the newly added device.
 *
 * @param dev The serial port device.
 *
 */
static int grlib_uart_dev_add(ddf_dev_t *dev)
{
	grlib_uart_t *ns = NULL;
	ddf_fun_t *fun = NULL;
	bool need_cleanup = false;
	bool need_unreg_intr_handler = false;
	int rc;
	
	ddf_msg(LVL_DEBUG, "grlib_uart_dev_add %s (handle = %d)",
	    ddf_dev_get_name(dev), (int) ddf_dev_get_handle(dev));
	
	/* Allocate soft-state for the device */
	ns = ddf_dev_data_alloc(dev, sizeof(grlib_uart_t));
	if (ns == NULL) {
		rc = ENOMEM;
		goto fail;
	}
	
	fibril_mutex_initialize(&ns->mutex);
	fibril_condvar_initialize(&ns->input_buffer_available);
	ns->dev = dev;
	
	rc = grlib_uart_dev_initialize(ns);
	if (rc != EOK)
		goto fail;
	
	need_cleanup = true;
	
	if (!grlib_uart_pio_enable(ns)) {
		rc = EADDRNOTAVAIL;
		goto fail;
	}
	
	/* Find out whether the device is present. */
	if (!grlib_uart_dev_probe(ns)) {
		rc = ENOENT;
		goto fail;
	}
	
	/* Serial port initialization (baud rate etc.). */
	grlib_uart_initialize_port(ns);
	
	/* Register interrupt handler. */
	if (grlib_uart_register_interrupt_handler(ns) != EOK) {
		ddf_msg(LVL_ERROR, "Failed to register interrupt handler.");
		rc = EADDRNOTAVAIL;
		goto fail;
	}
	need_unreg_intr_handler = true;
	
	/* Enable interrupt. */
	rc = grlib_uart_interrupt_enable(ns);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to enable the interrupt. Error code = "
		    "%d.", rc);
		goto fail;
	}
	
	fun = ddf_fun_create(dev, fun_exposed, "a");
	if (fun == NULL) {
		ddf_msg(LVL_ERROR, "Failed creating function.");
		goto fail;
	}
	
	/* Set device operations. */
	ddf_fun_set_ops(fun, &grlib_uart_dev_ops);
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
		grlib_uart_unregister_interrupt_handler(ns);
	
	if (need_cleanup)
		grlib_uart_dev_cleanup(ns);
	
	return rc;
}

static int grlib_uart_dev_remove(ddf_dev_t *dev)
{
	grlib_uart_t *ns = dev_grlib_uart(dev);
	
	fibril_mutex_lock(&ns->mutex);
	if (ns->client_connections > 0) {
		fibril_mutex_unlock(&ns->mutex);
		return EBUSY;
	}
	ns->removed = true;
	fibril_mutex_unlock(&ns->mutex);
	
	int rc = ddf_fun_unbind(ns->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to unbind function.");
		return rc;
	}
	
	ddf_fun_destroy(ns->fun);
	
	grlib_uart_port_cleanup(ns);
	grlib_uart_unregister_interrupt_handler(ns);
	grlib_uart_dev_cleanup(ns);
	return EOK;
}

/** Open the device.
 *
 * This is a callback function called when a client tries to connect to the
 * device.
 *
 * @param dev The device.
 *
 */
static int grlib_uart_open(ddf_fun_t *fun)
{
	grlib_uart_t *ns = fun_grlib_uart(fun);
	int res;
	
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
 * @param dev The device.
 *
 */
static void grlib_uart_close(ddf_fun_t *fun)
{
	grlib_uart_t *data = fun_grlib_uart(fun);
	
	fibril_mutex_lock(&data->mutex);
	
	assert(data->client_connections > 0);
	
	if (!(--data->client_connections))
		buf_clear(&data->input_buffer);
	
	fibril_mutex_unlock(&data->mutex);
}

/** Default handler for client requests which are not handled by the standard
 * interfaces.
 *
 * Configure the parameters of the serial communication.
 *
 */
static void grlib_uart_default_handler(ddf_fun_t *fun, ipc_callid_t callid,
    ipc_call_t *call)
{
	sysarg_t method = IPC_GET_IMETHOD(*call);
	
	switch (method) {
	default:
		async_answer_0(callid, ENOTSUP);
	}
}

/** Initialize the serial port driver.
 *
 * Initialize device operations structures with callback methods for handling
 * client requests to the serial port devices.
 *
 */
static void grlib_uart_init(void)
{
	ddf_log_init(NAME);
	
	grlib_uart_dev_ops.open = &grlib_uart_open;
	grlib_uart_dev_ops.close = &grlib_uart_close;
	
	grlib_uart_dev_ops.interfaces[CHAR_DEV_IFACE] = &grlib_uart_char_dev_ops;
	grlib_uart_dev_ops.default_handler = &grlib_uart_default_handler;
}

int main(int argc, char *argv[])
{
	printf("%s: HelenOS serial port driver\n", NAME);
	grlib_uart_init();
	return ddf_driver_main(&grlib_uart_driver);
}

/**
 * @}
 */
