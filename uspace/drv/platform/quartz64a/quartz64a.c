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

/** @addtogroup quartz64a
 * @{
 */
/** @file Quartz64A driver.
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>

#include <ddf/driver.h>
#include <ddf/log.h>
#include <ops/hw_res.h>
#include <ops/pio_window.h>
#include <irc.h>

#define NAME  "quartz64a"

enum {
	/* Interrupt controller. */
	quartz64a_ic_distr_membase = 0xfd400000,
	quartz64a_ic_distr_memsize = 0x00010000,
	quartz64a_ic_redist_membase = 0xfd460000,
	quartz64a_ic_redist_memsize = 0x00080000,
	/* UART. */
	quartz64a_uart_irq = 150,
	quartz64a_uart_membase = 0xfe660000,
	quartz64a_uart_memsize = 0x00001000,
};

typedef struct quartz64a_fun {
	hw_resource_list_t hw_resources;
} quartz64a_fun_t;

static errno_t quartz64a_dev_add(ddf_dev_t *);

static driver_ops_t quartz64a_ops = {
	.dev_add = &quartz64a_dev_add
};

static driver_t quartz64a_driver = {
	.name = NAME,
	.driver_ops = &quartz64a_ops
};

static hw_resource_t quartz64a_ic_res[] = {
	{
		.type = MEM_RANGE,
		.res.mem_range = {
			.address = quartz64a_ic_distr_membase,
			.size = quartz64a_ic_distr_memsize,
			.relative = false,
			.endianness = LITTLE_ENDIAN
		}
	},
	{
		.type = MEM_RANGE,
		.res.mem_range = {
			.address = quartz64a_ic_redist_membase,
			.size = quartz64a_ic_redist_memsize,
			.relative = false,
			.endianness = LITTLE_ENDIAN
		}
	}
};

static hw_resource_t quartz64a_uart_res[] = {
	{
		.type = MEM_RANGE,
		.res.mem_range = {
			.address = quartz64a_uart_membase,
			.size = quartz64a_uart_memsize,
			.relative = false,
			.endianness = LITTLE_ENDIAN
		}
	},
	{
		.type = INTERRUPT,
		.res.interrupt = {
			.irq = quartz64a_uart_irq
		}
	}
};

static pio_window_t quartz64a_pio_window = {
	.mem = {
		.base = 0,
		.size = -1
	}
};

static quartz64a_fun_t quartz64a_ic_fun_proto = {
	.hw_resources = {
		sizeof(quartz64a_ic_res) / sizeof(quartz64a_ic_res[0]),
		quartz64a_ic_res
	},
};

static quartz64a_fun_t quartz64a_uart_fun_proto = {
	.hw_resources = {
		sizeof(quartz64a_uart_res) / sizeof(quartz64a_uart_res[0]),
		quartz64a_uart_res
	},
};

/** Obtain function soft-state from DDF function node. */
static quartz64a_fun_t *quartz64a_fun(ddf_fun_t *fnode)
{
	return ddf_fun_data_get(fnode);
}

static hw_resource_list_t *quartz64a_get_resources(ddf_fun_t *fnode)
{
	quartz64a_fun_t *fun = quartz64a_fun(fnode);

	assert(fun != NULL);
	return &fun->hw_resources;
}

static bool quartz64a_fun_owns_interrupt(quartz64a_fun_t *fun, int irq)
{

	const hw_resource_list_t *res = &fun->hw_resources;

	/* Check that specified irq really belongs to the function */
	for (size_t i = 0; i < res->count; ++i) {
		if (res->resources[i].type == INTERRUPT &&
		    res->resources[i].res.interrupt.irq == irq) {
			return true;
		}
	}

	return false;
}

static errno_t quartz64a_enable_interrupt(ddf_fun_t *fun, int irq)
{
	quartz64a_fun_t *qfun = quartz64a_fun(fun);

	if (!quartz64a_fun_owns_interrupt(qfun, irq))
		return EINVAL;

	return irc_enable_interrupt(irq);
}

static pio_window_t *quartz64a_get_pio_window(ddf_fun_t *fnode)
{
	return &quartz64a_pio_window;
}

static hw_res_ops_t quartz64a_hw_res_ops = {
	.get_resource_list = &quartz64a_get_resources,
	.enable_interrupt = &quartz64a_enable_interrupt,
};

static pio_window_ops_t quartz64a_pio_window_ops = {
	.get_pio_window = &quartz64a_get_pio_window
};

static ddf_dev_ops_t quartz64a_fun_ops = {
	.interfaces = {
		[HW_RES_DEV_IFACE] = &quartz64a_hw_res_ops,
		[PIO_WINDOW_DEV_IFACE] = &quartz64a_pio_window_ops
	}
};

static errno_t quartz64a_add_fun(ddf_dev_t *dev, const char *name,
    const char *str_match_id, quartz64a_fun_t *fun_proto)
{
	ddf_msg(LVL_NOTE, "Adding function '%s'.", name);

	ddf_fun_t *fnode = NULL;
	errno_t rc;

	/* Create new device. */
	fnode = ddf_fun_create(dev, fun_inner, name);
	if (fnode == NULL) {
		ddf_msg(LVL_ERROR, "Error creating function '%s'.", name);
		rc = ENOMEM;
		goto error;
	}

	quartz64a_fun_t *fun = ddf_fun_data_alloc(fnode,
	    sizeof(quartz64a_fun_t));
	if (fun == NULL) {
		rc = ENOMEM;
		goto error;
	}
	*fun = *fun_proto;

	/* Add match ID. */
	rc = ddf_fun_add_match_id(fnode, str_match_id, 100);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error adding match ID to function '%s'.",
		    name);
		goto error;
	}

	/* Set provided operations to the device. */
	ddf_fun_set_ops(fnode, &quartz64a_fun_ops);

	/* Register function. */
	rc = ddf_fun_bind(fnode);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function '%s'.", name);
		goto error;
	}

	return EOK;

error:
	if (fnode != NULL)
		ddf_fun_destroy(fnode);

	return rc;
}

static errno_t quartz64a_add_functions(ddf_dev_t *dev)
{
	errno_t rc;

	rc = quartz64a_add_fun(dev, "intctl", "arm/gicv3",
	    &quartz64a_ic_fun_proto);
	if (rc != EOK)
		return rc;

	rc = quartz64a_add_fun(dev, "uart", "arm/dwapb",
	    &quartz64a_uart_fun_proto);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Add device. */
static errno_t quartz64a_dev_add(ddf_dev_t *dev)
{
	ddf_msg(LVL_NOTE, "quartz64a_dev_add(), device=%s.",
	    ddf_dev_get_name(dev));

	/* Register functions. */
	if (quartz64a_add_functions(dev))
		ddf_msg(LVL_ERROR, "Failed to add functions for Quartz64 model A "
		    "platform.");

	return EOK;
}

int main(int argc, char *argv[])
{
	errno_t rc;

	printf("%s: HelenOS Quartz64 model A platform driver\n", NAME);

	rc = ddf_log_init(NAME);
	if (rc != EOK) {
		printf("%s: Failed connecting logging service.", NAME);
		return 1;
	}

	return ddf_driver_main(&quartz64a_driver);
}

/** @}
 */
