/*
 * Copyright (c) 2021 Martin Decky
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
 * @addtogroup hikey960
 * @{
 */

/** @file
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

#define NAME  "hikey960"

enum {
	hikey960_ic_distr_membase = 0xE82B1000,
	hikey960_ic_distr_memsize = 0x00001000,
	hikey960_ic_cpui_membase = 0xE82B2000,
	hikey960_ic_cpui_memsize = 0x00001004,
	hikey960_uart_irq = 111,
	hikey960_uart_membase = 0xFFF32000,
	hikey960_uart_memsize = 0x00001000
};

typedef struct hikey960_fun {
	hw_resource_list_t hw_resources;
} hikey960_fun_t;

static errno_t hikey960_dev_add(ddf_dev_t *);

static driver_ops_t hikey960_ops = {
	.dev_add = &hikey960_dev_add
};

static driver_t hikey960_driver = {
	.name = NAME,
	.driver_ops = &hikey960_ops
};

static hw_resource_t hikey960_ic_res[] = {
	{
		.type = MEM_RANGE,
		.res.mem_range = {
			.address = hikey960_ic_distr_membase,
			.size = hikey960_ic_distr_memsize,
			.relative = false,
			.endianness = LITTLE_ENDIAN
		}
	},
	{
		.type = MEM_RANGE,
		.res.mem_range = {
			.address = hikey960_ic_cpui_membase,
			.size = hikey960_ic_cpui_memsize,
			.relative = false,
			.endianness = LITTLE_ENDIAN
		}
	}
};

static hw_resource_t hikey960_uart_res[] = {
	{
		.type = MEM_RANGE,
		.res.mem_range = {
			.address = hikey960_uart_membase,
			.size = hikey960_uart_memsize,
			.relative = false,
			.endianness = LITTLE_ENDIAN
		}
	},
	{
		.type = INTERRUPT,
		.res.interrupt = {
			.irq = hikey960_uart_irq
		}
	}
};

static pio_window_t hikey960_pio_window = {
	.mem = {
		.base = 0,
		.size = -1
	}
};

static hikey960_fun_t hikey960_ic_fun_proto = {
	.hw_resources = {
		sizeof(hikey960_ic_res) / sizeof(hikey960_ic_res[0]),
		hikey960_ic_res
	},
};

static hikey960_fun_t hikey960_uart_fun_proto = {
	.hw_resources = {
		sizeof(hikey960_uart_res) / sizeof(hikey960_uart_res[0]),
		hikey960_uart_res
	},
};

/** Obtain function soft-state from DDF function node. */
static hikey960_fun_t *hikey960_fun(ddf_fun_t *fnode)
{
	return ddf_fun_data_get(fnode);
}

static hw_resource_list_t *hikey960_get_resources(ddf_fun_t *fnode)
{
	hikey960_fun_t *fun = hikey960_fun(fnode);

	assert(fun != NULL);
	return &fun->hw_resources;
}

static errno_t hikey960_enable_interrupt(ddf_fun_t *fun, int irq)
{
	/* TODO */
	return false;
}

static pio_window_t *hikey960_get_pio_window(ddf_fun_t *fnode)
{
	return &hikey960_pio_window;
}

static hw_res_ops_t hikey960_hw_res_ops = {
	.get_resource_list = &hikey960_get_resources,
	.enable_interrupt = &hikey960_enable_interrupt,
};

static pio_window_ops_t hikey960_pio_window_ops = {
	.get_pio_window = &hikey960_get_pio_window
};

static ddf_dev_ops_t hikey960_fun_ops = {
	.interfaces = {
		[HW_RES_DEV_IFACE] = &hikey960_hw_res_ops,
		[PIO_WINDOW_DEV_IFACE] = &hikey960_pio_window_ops
	}
};

static errno_t hikey960_add_fun(ddf_dev_t *dev, const char *name,
    const char *str_match_id, hikey960_fun_t *fun_proto)
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

	hikey960_fun_t *fun = ddf_fun_data_alloc(fnode,
	    sizeof(hikey960_fun_t));
	*fun = *fun_proto;

	/* Add match ID. */
	rc = ddf_fun_add_match_id(fnode, str_match_id, 100);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error adding match ID to function '%s'.",
		    name);
		goto error;
	}

	/* Set provided operations to the device. */
	ddf_fun_set_ops(fnode, &hikey960_fun_ops);

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

static errno_t hikey960_add_functions(ddf_dev_t *dev)
{
	errno_t rc;

	rc = hikey960_add_fun(dev, "intctl", "arm/gicv2",
	    &hikey960_ic_fun_proto);
	if (rc != EOK)
		return rc;

	rc = hikey960_add_fun(dev, "uart", "arm/pl011",
	    &hikey960_uart_fun_proto);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Add device. */
static errno_t hikey960_dev_add(ddf_dev_t *dev)
{
	ddf_msg(LVL_NOTE, "hikey960_dev_add(), device=%s.",
	    ddf_dev_get_name(dev));

	/* Register functions. */
	if (hikey960_add_functions(dev))
		ddf_msg(LVL_ERROR, "Failed to add functions for HiKey 960 platform.");

	return EOK;
}

int main(int argc, char *argv[])
{
	errno_t rc;

	printf("%s: HelenOS HiKey 960 platform driver\n", NAME);

	rc = ddf_log_init(NAME);
	if (rc != EOK) {
		printf("%s: Failed connecting logging service.", NAME);
		return 1;
	}

	return ddf_driver_main(&hikey960_driver);
}

/**
 * @}
 */
