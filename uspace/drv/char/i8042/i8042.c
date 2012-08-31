/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * Copyright (c) 2006 Josef Cejka
 * Copyright (c) 2009 Jiri Svoboda
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

#include <device/hw_res.h>
#include <ddi.h>
#include <libarch/ddi.h>
#include <errno.h>
#include <str_error.h>
#include <inttypes.h>
#include <ddf/log.h>
#include <ddf/interrupt.h>
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

#define CHECK_RET_DESTROY(ret, msg...) \
	do { \
		if (ret != EOK) { \
			ddf_msg(LVL_ERROR, msg); \
			if (dev->kbd_fun) { \
				dev->kbd_fun->driver_data = NULL; \
				ddf_fun_destroy(dev->kbd_fun); \
			} \
			if (dev->aux_fun) { \
				dev->aux_fun->driver_data = NULL; \
				ddf_fun_destroy(dev->aux_fun); \
			} \
		} \
	} while (0)

#define CHECK_RET_UNBIND_DESTROY(ret, msg...) \
	do { \
		if (ret != EOK) { \
			ddf_msg(LVL_ERROR, msg); \
			if (dev->kbd_fun) { \
				ddf_fun_unbind(dev->kbd_fun); \
				dev->kbd_fun->driver_data = NULL; \
				ddf_fun_destroy(dev->kbd_fun); \
			} \
			if (dev->aux_fun) { \
				ddf_fun_unbind(dev->aux_fun); \
				dev->aux_fun->driver_data = NULL; \
				ddf_fun_destroy(dev->aux_fun); \
			} \
		} \
	} while (0)

void default_handler(ddf_fun_t *, ipc_callid_t, ipc_call_t *);

/** Port function operations. */
static ddf_dev_ops_t ops = {
	.default_handler = default_handler,
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

/** Get i8042 soft state from device node. */
static i8042_t *dev_i8042(ddf_dev_t *dev)
{
	return ddf_dev_data_get(dev);
}

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
 * @param dev  Device that caued the interrupt.
 * @param iid  Call id.
 * @param call pointerr to call data.
 *
 */
static void i8042_irq_handler(ddf_dev_t *dev, ipc_callid_t iid,
    ipc_call_t *call)
{
	i8042_t *controller = dev_i8042(dev);
	
	const uint8_t status = IPC_GET_ARG1(*call);
	const uint8_t data = IPC_GET_ARG2(*call);
	
	buffer_t *buffer = (status & i8042_AUX_DATA) ?
	    &controller->aux_buffer : &controller->kbd_buffer;
	
	buffer_write(buffer, data);
}

/** Initialize i8042 driver structure.
 *
 * @param dev       Driver structure to initialize.
 * @param regs      I/O address of registers.
 * @param reg_size  size of the reserved I/O address space.
 * @param irq_kbd   IRQ for primary port.
 * @param irq_mouse IRQ for aux port.
 * @param ddf_dev   DDF device structure of the device.
 *
 * @return Error code.
 *
 */
int i8042_init(i8042_t *dev, void *regs, size_t reg_size, int irq_kbd,
    int irq_mouse, ddf_dev_t *ddf_dev)
{
	const size_t range_count = sizeof(i8042_ranges) /
	    sizeof(irq_pio_range_t);
	irq_pio_range_t ranges[range_count];
	const size_t cmd_count = sizeof(i8042_cmds) / sizeof(irq_cmd_t);
	irq_cmd_t cmds[cmd_count];

	int rc;
	bool kbd_bound = false;
	bool aux_bound = false;

	dev->kbd_fun = NULL;
	dev->aux_fun = NULL;
	
	if (reg_size < sizeof(i8042_regs_t)) {
		rc = EINVAL;
		goto error;
	}
	
	if (pio_enable(regs, sizeof(i8042_regs_t), (void **) &dev->regs) != 0) {
		rc = EIO;
		goto error;
	}
	
	dev->kbd_fun = ddf_fun_create(ddf_dev, fun_inner, "ps2a");
	if (dev->kbd_fun == NULL) {
		rc = ENOMEM;
		goto error;
	};
	
	rc = ddf_fun_add_match_id(dev->kbd_fun, "char/xtkbd", 90);
	if (rc != EOK)
		goto error;
	
	dev->aux_fun = ddf_fun_create(ddf_dev, fun_inner, "ps2b");
	if (dev->aux_fun == NULL) {
		rc = ENOMEM;
		goto error;
	}
	
	rc = ddf_fun_add_match_id(dev->aux_fun, "char/ps2mouse", 90);
	if (rc != EOK)
		goto error;
	
	ddf_fun_set_ops(dev->kbd_fun, &ops);
	ddf_fun_set_ops(dev->aux_fun, &ops);
	
	buffer_init(&dev->kbd_buffer, dev->kbd_data, BUFFER_SIZE);
	buffer_init(&dev->aux_buffer, dev->aux_data, BUFFER_SIZE);
	fibril_mutex_initialize(&dev->write_guard);
	
	rc = ddf_fun_bind(dev->kbd_fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to bind keyboard function: %s.",
		    ddf_fun_get_name(dev->kbd_fun));
		goto error;
	}
	kbd_bound = true;
	
	rc = ddf_fun_bind(dev->aux_fun);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to bind aux function: %s.",
		    ddf_fun_get_name(dev->aux_fun));
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
	ranges[0].base = (uintptr_t) regs;

	memcpy(cmds, i8042_cmds, sizeof(i8042_cmds));
	cmds[0].addr = (void *) &(((i8042_regs_t *) regs)->status);
	cmds[3].addr = (void *) &(((i8042_regs_t *) regs)->data);

	irq_code_t irq_code = {
		.rangecount = range_count,
		.ranges = ranges,
		.cmdcount = cmd_count,
		.cmds = cmds
	};
	
	rc = register_interrupt_handler(ddf_dev, irq_kbd, i8042_irq_handler,
	    &irq_code);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed set handler for kbd: %s.",
		    ddf_dev_get_name(ddf_dev));
		goto error;
	}
	
	rc = register_interrupt_handler(ddf_dev, irq_mouse, i8042_irq_handler,
	    &irq_code);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed set handler for mouse: %s.",
		    ddf_dev_get_name(ddf_dev));
		goto error;
	}
	
	/* Enable interrupts */
	async_sess_t *parent_sess = ddf_dev_parent_sess_get(ddf_dev);
	assert(parent_sess != NULL);
	
	const bool enabled = hw_res_enable_interrupt(parent_sess);
	if (!enabled) {
		log_msg(LVL_ERROR, "Failed to enable interrupts: %s.",
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
		ddf_fun_unbind(dev->kbd_fun);
	if (aux_bound)
		ddf_fun_unbind(dev->aux_fun);
	if (dev->kbd_fun != NULL)
		ddf_fun_destroy(dev->kbd_fun);
	if (dev->aux_fun != NULL)
		ddf_fun_destroy(dev->aux_fun);

	return rc;
}

// FIXME TODO use shared instead this
enum {
	IPC_CHAR_READ = DEV_FIRST_CUSTOM_METHOD,
	IPC_CHAR_WRITE,
};

/** Write data to i8042 port.
 *
 * @param fun    DDF function.
 * @param buffer Data source.
 * @param size   Data size.
 *
 * @return Bytes written.
 *
 */
static int i8042_write(ddf_fun_t *fun, char *buffer, size_t size)
{
	i8042_t *controller = dev_i8042(ddf_fun_get_dev(fun));
	fibril_mutex_lock(&controller->write_guard);
	
	for (size_t i = 0; i < size; ++i) {
		if (controller->aux_fun == fun) {
			wait_ready(controller);
			pio_write_8(&controller->regs->status,
			    i8042_CMD_WRITE_AUX);
		}
		
		wait_ready(controller);
		pio_write_8(&controller->regs->data, buffer[i]);
	}
	
	fibril_mutex_unlock(&controller->write_guard);
	return size;
}

/** Read data from i8042 port.
 *
 * @param fun    DDF function.
 * @param buffer Data place.
 * @param size   Data place size.
 *
 * @return Bytes read.
 *
 */
static int i8042_read(ddf_fun_t *fun, char *data, size_t size)
{
	i8042_t *controller = dev_i8042(ddf_fun_get_dev(fun));
	buffer_t *buffer = (fun == controller->aux_fun) ?
	    &controller->aux_buffer : &controller->kbd_buffer;
	
	for (size_t i = 0; i < size; ++i)
		*data++ = buffer_read(buffer);
	
	return size;
}

/** Handle data requests.
 *
 * @param fun  ddf_fun_t function.
 * @param id   callid
 * @param call IPC request.
 *
 */
void default_handler(ddf_fun_t *fun, ipc_callid_t id, ipc_call_t *call)
{
	const sysarg_t method = IPC_GET_IMETHOD(*call);
	const size_t size = IPC_GET_ARG1(*call);
	
	switch (method) {
	case IPC_CHAR_READ:
		if (size <= 4 * sizeof(sysarg_t)) {
			sysarg_t message[4] = {};
			
			i8042_read(fun, (char *) message, size);
			async_answer_4(id, size, message[0], message[1],
			    message[2], message[3]);
		} else
			async_answer_0(id, ELIMIT);
		break;
	
	case IPC_CHAR_WRITE:
		if (size <= 3 * sizeof(sysarg_t)) {
			const sysarg_t message[3] = {
				IPC_GET_ARG2(*call),
				IPC_GET_ARG3(*call),
				IPC_GET_ARG4(*call)
			};
			
			i8042_write(fun, (char *) message, size);
			async_answer_0(id, size);
		} else
			async_answer_0(id, ELIMIT);
	
	default:
		async_answer_0(id, EINVAL);
	}
}

/**
 * @}
 */
