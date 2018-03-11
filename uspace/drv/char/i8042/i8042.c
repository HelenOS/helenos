/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * Copyright (c) 2006 Josef Cejka
 * Copyright (c) 2017 Jiri Svoboda
 * Copyright (c) 2011 Jan Vesely
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

/** @addtogroup kbd_port
 * @ingroup kbd
 * @{
 */

/** @file
 * @brief i8042 PS/2 port driver.
 */

#include <adt/circ_buf.h>
#include <ddf/log.h>
#include <ddf/interrupt.h>
#include <ddi.h>
#include <device/hw_res.h>
#include <errno.h>
#include <str_error.h>
#include <inttypes.h>
#include <io/chardev_srv.h>

#include "i8042.h"

/* Interesting bits for status register */
#define i8042_OUTPUT_FULL  0x01
#define i8042_INPUT_FULL   0x02
#define i8042_AUX_DATA     0x20

/* Command constants */
#define i8042_CMD_WRITE_CMDB  0x60  /**< Write command byte */
#define i8042_CMD_WRITE_AUX   0xd4  /**< Write aux device */

/* Command byte fields */
#define i8042_KBD_IE         0x01
#define i8042_AUX_IE         0x02
#define i8042_KBD_DISABLE    0x10
#define i8042_AUX_DISABLE    0x20
#define i8042_KBD_TRANSLATE  0x40  /* Use this to switch to XT scancodes */

static void i8042_char_conn(ipc_callid_t, ipc_call_t *, void *);
static errno_t i8042_read(chardev_srv_t *, void *, size_t, size_t *);
static errno_t i8042_write(chardev_srv_t *, const void *, size_t, size_t *);

static chardev_ops_t i8042_chardev_ops = {
	.read = i8042_read,
	.write = i8042_write
};

static const irq_pio_range_t i8042_ranges[] = {
	{
		.base = 0,
		.size = sizeof(i8042_regs_t)
	}
};

/** i8042 Interrupt pseudo-code. */
static const irq_cmd_t i8042_cmds[] = {
	{
		.cmd = CMD_PIO_READ_8,
		.addr = NULL,  /* will be patched in run-time */
		.dstarg = 1
	},
	{
		.cmd = CMD_AND,
		.value = i8042_OUTPUT_FULL,
		.srcarg = 1,
		.dstarg = 3
	},
	{
		.cmd = CMD_PREDICATE,
		.value = 2,
		.srcarg = 3
	},
	{
		.cmd = CMD_PIO_READ_8,
		.addr = NULL,  /* will be patched in run-time */
		.dstarg = 2
	},
	{
		.cmd = CMD_ACCEPT
	}
};

/** Wait until it is safe to write to the device. */
static void wait_ready(i8042_t *dev)
{
	assert(dev);
	while (pio_read_8(&dev->regs->status) & i8042_INPUT_FULL);
}

/** Interrupt handler routine.
 *
 * Write new data to the corresponding buffer.
 *
 * @param call pointerr to call data.
 * @param dev  Device that caued the interrupt.
 *
 */
static void i8042_irq_handler(ipc_call_t *call, ddf_dev_t *dev)
{
	i8042_t *controller = ddf_dev_data_get(dev);
	errno_t rc;

	const uint8_t status = IPC_GET_ARG1(*call);
	const uint8_t data = IPC_GET_ARG2(*call);

	i8042_port_t *port = (status & i8042_AUX_DATA) ?
	    controller->aux : controller->kbd;

	fibril_mutex_lock(&port->buf_lock);

	rc = circ_buf_push(&port->cbuf, &data);
	if (rc != EOK)
		ddf_msg(LVL_ERROR, "Buffer overrun");

	fibril_mutex_unlock(&port->buf_lock);
	fibril_condvar_broadcast(&port->buf_cv);
}

/** Initialize i8042 driver structure.
 *
 * @param dev       Driver structure to initialize.
 * @param regs      I/O range  of registers.
 * @param irq_kbd   IRQ for primary port.
 * @param irq_mouse IRQ for aux port.
 * @param ddf_dev   DDF device structure of the device.
 *
 * @return Error code.
 *
 */
errno_t i8042_init(i8042_t *dev, addr_range_t *regs, int irq_kbd,
    int irq_mouse, ddf_dev_t *ddf_dev)
{
	const size_t range_count = sizeof(i8042_ranges) /
	    sizeof(irq_pio_range_t);
	irq_pio_range_t ranges[range_count];
	const size_t cmd_count = sizeof(i8042_cmds) / sizeof(irq_cmd_t);
	irq_cmd_t cmds[cmd_count];
	ddf_fun_t *kbd_fun;
	ddf_fun_t *aux_fun;
	i8042_regs_t *ar;

	errno_t rc;
	bool kbd_bound = false;
	bool aux_bound = false;

	if (regs->size < sizeof(i8042_regs_t)) {
		rc = EINVAL;
		goto error;
	}

	if (pio_enable_range(regs, (void **) &dev->regs) != 0) {
		rc = EIO;
		goto error;
	}

	kbd_fun = ddf_fun_create(ddf_dev, fun_inner, "ps2a");
	if (kbd_fun == NULL) {
		rc = ENOMEM;
		goto error;
	}

	dev->kbd = ddf_fun_data_alloc(kbd_fun, sizeof(i8042_port_t));
	if (dev->kbd == NULL) {
		rc = ENOMEM;
		goto error;
	}

	dev->kbd->fun = kbd_fun;
	dev->kbd->ctl = dev;
	chardev_srvs_init(&dev->kbd->cds);
	dev->kbd->cds.ops = &i8042_chardev_ops;
	dev->kbd->cds.sarg = dev->kbd;
	fibril_mutex_initialize(&dev->kbd->buf_lock);
	fibril_condvar_initialize(&dev->kbd->buf_cv);

	rc = ddf_fun_add_match_id(dev->kbd->fun, "char/xtkbd", 90);
	if (rc != EOK)
		goto error;

	aux_fun = ddf_fun_create(ddf_dev, fun_inner, "ps2b");
	if (aux_fun == NULL) {
		rc = ENOMEM;
		goto error;
	}

	dev->aux = ddf_fun_data_alloc(aux_fun, sizeof(i8042_port_t));
	if (dev->aux == NULL) {
		rc = ENOMEM;
		goto error;
	}

	dev->aux->fun = aux_fun;
	dev->aux->ctl = dev;
	chardev_srvs_init(&dev->aux->cds);
	dev->aux->cds.ops = &i8042_chardev_ops;
	dev->aux->cds.sarg = dev->aux;
	fibril_mutex_initialize(&dev->aux->buf_lock);
	fibril_condvar_initialize(&dev->aux->buf_cv);

	rc = ddf_fun_add_match_id(dev->aux->fun, "char/ps2mouse", 90);
	if (rc != EOK)
		goto error;

	ddf_fun_set_conn_handler(dev->kbd->fun, i8042_char_conn);
	ddf_fun_set_conn_handler(dev->aux->fun, i8042_char_conn);

	circ_buf_init(&dev->kbd->cbuf, dev->kbd->buf_data, BUFFER_SIZE, 1);
	circ_buf_init(&dev->aux->cbuf, dev->aux->buf_data, BUFFER_SIZE, 1);
	fibril_mutex_initialize(&dev->write_guard);

	rc = ddf_fun_bind(dev->kbd->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to bind keyboard function: %s.",
		    ddf_fun_get_name(dev->kbd->fun));
		goto error;
	}
	kbd_bound = true;

	rc = ddf_fun_bind(dev->aux->fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to bind aux function: %s.",
		    ddf_fun_get_name(dev->aux->fun));
		goto error;
	}
	aux_bound = true;

	/* Disable kbd and aux */
	wait_ready(dev);
	pio_write_8(&dev->regs->status, i8042_CMD_WRITE_CMDB);
	wait_ready(dev);
	pio_write_8(&dev->regs->data, i8042_KBD_DISABLE | i8042_AUX_DISABLE);

	/* Flush all current IO */
	while (pio_read_8(&dev->regs->status) & i8042_OUTPUT_FULL)
		(void) pio_read_8(&dev->regs->data);

	memcpy(ranges, i8042_ranges, sizeof(i8042_ranges));
	ranges[0].base = RNGABS(*regs);


	ar = RNGABSPTR(*regs);
	memcpy(cmds, i8042_cmds, sizeof(i8042_cmds));
	cmds[0].addr = (void *) &ar->status;
	cmds[3].addr = (void *) &ar->data;

	irq_code_t irq_code = {
		.rangecount = range_count,
		.ranges = ranges,
		.cmdcount = cmd_count,
		.cmds = cmds
	};

	int irq_kbd_cap;
	rc = register_interrupt_handler(ddf_dev, irq_kbd,
	    i8042_irq_handler, &irq_code, &irq_kbd_cap);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed set handler for kbd: %s.",
		    ddf_dev_get_name(ddf_dev));
		goto error;
	}

	int irq_mouse_cap;
	rc = register_interrupt_handler(ddf_dev, irq_mouse,
	    i8042_irq_handler, &irq_code, &irq_mouse_cap);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed set handler for mouse: %s.",
		    ddf_dev_get_name(ddf_dev));
		goto error;
	}

	/* Enable interrupts */
	async_sess_t *parent_sess = ddf_dev_parent_sess_get(ddf_dev);
	assert(parent_sess != NULL);

	rc = hw_res_enable_interrupt(parent_sess, irq_kbd);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed to enable keyboard interrupt: %s.",
		    ddf_dev_get_name(ddf_dev));
		rc = EIO;
		goto error;
	}

	rc = hw_res_enable_interrupt(parent_sess, irq_mouse);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed to enable mouse interrupt: %s.",
		    ddf_dev_get_name(ddf_dev));
		rc = EIO;
		goto error;
	}

	/* Enable port interrupts. */
	wait_ready(dev);
	pio_write_8(&dev->regs->status, i8042_CMD_WRITE_CMDB);
	wait_ready(dev);
	pio_write_8(&dev->regs->data, i8042_KBD_IE | i8042_KBD_TRANSLATE |
	    i8042_AUX_IE);

	return EOK;
error:
	if (kbd_bound)
		ddf_fun_unbind(dev->kbd->fun);
	if (aux_bound)
		ddf_fun_unbind(dev->aux->fun);
	if (dev->kbd->fun != NULL)
		ddf_fun_destroy(dev->kbd->fun);
	if (dev->aux->fun != NULL)
		ddf_fun_destroy(dev->aux->fun);

	return rc;
}

/** Write data to i8042 port.
 *
 * @param srv	 Connection-specific data
 * @param buffer Data source
 * @param size   Data size
 * @param nwr    Place to store number of bytes successfully written
 *
 * @return EOK on success or non-zero error code
 *
 */
static errno_t i8042_write(chardev_srv_t *srv, const void *data, size_t size,
    size_t *nwr)
{
	i8042_port_t *port = (i8042_port_t *)srv->srvs->sarg;
	i8042_t *i8042 = port->ctl;
	const char *dp = (const char *)data;

	fibril_mutex_lock(&i8042->write_guard);

	for (size_t i = 0; i < size; ++i) {
		if (port == i8042->aux) {
			wait_ready(i8042);
			pio_write_8(&i8042->regs->status,
			    i8042_CMD_WRITE_AUX);
		}

		wait_ready(i8042);
		pio_write_8(&i8042->regs->data, dp[i]);
	}

	fibril_mutex_unlock(&i8042->write_guard);
	*nwr = size;
	return EOK;
}

/** Read data from i8042 port.
 *
 * @param srv	 Connection-specific data
 * @param buffer Data place
 * @param size   Data place size
 * @param nread  Place to store number of bytes successfully read
 *
 * @return EOK on success or non-zero error code
 *
 */
static errno_t i8042_read(chardev_srv_t *srv, void *dest, size_t size,
    size_t *nread)
{
	i8042_port_t *port = (i8042_port_t *)srv->srvs->sarg;
	size_t p;
	uint8_t *destp = (uint8_t *)dest;
	errno_t rc;

	fibril_mutex_lock(&port->buf_lock);

	while (circ_buf_nused(&port->cbuf) == 0)
		fibril_condvar_wait(&port->buf_cv, &port->buf_lock);

	p = 0;
	while (p < size) {
		rc = circ_buf_pop(&port->cbuf, &destp[p]);
		if (rc != EOK)
			break;
		++p;
	}

	fibril_mutex_unlock(&port->buf_lock);

	*nread = p;
	return EOK;
}

/** Handle data requests.
 *
 * @param id   callid
 * @param call IPC request.
 * @param arg  ddf_fun_t function.
 */
void i8042_char_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	i8042_port_t *port = ddf_fun_data_get((ddf_fun_t *)arg);

	chardev_conn(iid, icall, &port->cds);
}

/**
 * @}
 */
