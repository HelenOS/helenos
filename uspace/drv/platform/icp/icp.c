/*
 * Copyright (c) 2014 Jiri Svoboda
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
 * @addtogroup icp
 * @{
 */

/** @file
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <irc.h>
#include <stdbool.h>
#include <stdlib.h>

#include <ddf/driver.h>
#include <ddf/log.h>
#include <ops/hw_res.h>
#include <ops/pio_window.h>

#define NAME "icp"

enum {
	icp_kbd_base = 0x18000000,
	icp_kbd_irq = 3,
	icp_mouse_base = 0x19000000,
	icp_mouse_irq = 4,
	icp_ic_base = 0x14000000
};

typedef struct icp_fun {
	hw_resource_list_t hw_resources;
} icp_fun_t;

static errno_t icp_dev_add(ddf_dev_t *dev);

static driver_ops_t icp_ops = {
	.dev_add = &icp_dev_add
};

static driver_t icp_driver = {
	.name = NAME,
	.driver_ops = &icp_ops
};

static hw_resource_t icp_kbd_res[] = {
	{
		.type = MEM_RANGE,
		.res.mem_range = {
			.address = icp_kbd_base,
			.size = 9,
			.relative = false,
			.endianness = LITTLE_ENDIAN
		}
	},
	{
		.type = INTERRUPT,
		.res.interrupt = {
			.irq = icp_kbd_irq
		}
	}
};

static hw_resource_t icp_mouse_res[] = {
	{
		.type = MEM_RANGE,
		.res.mem_range = {
			.address = icp_mouse_base,
			.size = 9,
			.relative = false,
			.endianness = LITTLE_ENDIAN
		}
	},
	{
		.type = INTERRUPT,
		.res.interrupt = {
			.irq = icp_mouse_irq
		}
	}
};

static hw_resource_t icp_ic_res[] = {
	{
		.type = MEM_RANGE,
		.res.mem_range = {
			.address = icp_ic_base,
			.size = 40,
			.relative = false,
			.endianness = LITTLE_ENDIAN
		}
	}
};

static pio_window_t icp_pio_window = {
	.mem = {
		.base = 0,
		.size = -1
	}
};

static icp_fun_t icp_kbd_fun_proto = {
	.hw_resources = {
		sizeof(icp_kbd_res) / sizeof(icp_kbd_res[0]),
		icp_kbd_res
	},
};

static icp_fun_t icp_mouse_fun_proto = {
	.hw_resources = {
		sizeof(icp_mouse_res) / sizeof(icp_mouse_res[0]),
		icp_mouse_res
	},
};

static icp_fun_t icp_ic_fun_proto = {
	.hw_resources = {
		sizeof(icp_ic_res) / sizeof(icp_ic_res[0]),
		icp_ic_res
	},
};

/** Obtain function soft-state from DDF function node */
static icp_fun_t *icp_fun(ddf_fun_t *fnode)
{
	return ddf_fun_data_get(fnode);
}

static hw_resource_list_t *icp_get_resources(ddf_fun_t *fnode)
{
	icp_fun_t *fun = icp_fun(fnode);

	assert(fun != NULL);
	return &fun->hw_resources;
}

static bool icp_fun_owns_interrupt(icp_fun_t *fun, int irq)
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

static errno_t icp_fun_enable_interrupt(ddf_fun_t *fnode, int irq)
{
	icp_fun_t *fun = icp_fun(fnode);

	if (!icp_fun_owns_interrupt(fun, irq))
		return EINVAL;

	return irc_enable_interrupt(irq);
}

static errno_t icp_fun_disable_interrupt(ddf_fun_t *fnode, int irq)
{
	icp_fun_t *fun = icp_fun(fnode);

	if (!icp_fun_owns_interrupt(fun, irq))
		return EINVAL;

	return irc_disable_interrupt(irq);
}

static errno_t icp_fun_clear_interrupt(ddf_fun_t *fnode, int irq)
{
	icp_fun_t *fun = icp_fun(fnode);

	if (!icp_fun_owns_interrupt(fun, irq))
		return EINVAL;

	return irc_clear_interrupt(irq);
}

static pio_window_t *icp_get_pio_window(ddf_fun_t *fnode)
{
	return &icp_pio_window;
}

static hw_res_ops_t icp_hw_res_ops = {
	.get_resource_list = &icp_get_resources,
	.enable_interrupt = &icp_fun_enable_interrupt,
	.disable_interrupt = &icp_fun_disable_interrupt,
	.clear_interrupt = &icp_fun_clear_interrupt
};

static pio_window_ops_t icp_pio_window_ops = {
	.get_pio_window = &icp_get_pio_window
};

static ddf_dev_ops_t icp_fun_ops = {
	.interfaces = {
		[HW_RES_DEV_IFACE] = &icp_hw_res_ops,
		[PIO_WINDOW_DEV_IFACE] = &icp_pio_window_ops
	}
};

static errno_t icp_add_fun(ddf_dev_t *dev, const char *name, const char *str_match_id,
    icp_fun_t *fun_proto)
{
	ddf_msg(LVL_NOTE, "Adding function '%s'.", name);

	ddf_fun_t *fnode = NULL;
	errno_t rc;

	/* Create new device. */
	fnode = ddf_fun_create(dev, fun_inner, name);
	if (fnode == NULL) {
		ddf_msg(LVL_ERROR, "Error creating function '%s'", name);
		rc = ENOMEM;
		goto error;
	}

	icp_fun_t *fun = ddf_fun_data_alloc(fnode, sizeof(icp_fun_t));
	*fun = *fun_proto;

	/* Add match ID */
	rc = ddf_fun_add_match_id(fnode, str_match_id, 100);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error adding match ID");
		goto error;
	}

	/* Set provided operations to the device. */
	ddf_fun_set_ops(fnode, &icp_fun_ops);

	/* Register function. */
	if (ddf_fun_bind(fnode) != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function %s.", name);
		goto error;
	}

	return EOK;

error:
	if (fnode != NULL)
		ddf_fun_destroy(fnode);

	return rc;
}

static errno_t icp_add_functions(ddf_dev_t *dev)
{
	errno_t rc;

	rc = icp_add_fun(dev, "intctl", "integratorcp/intctl",
	    &icp_ic_fun_proto);
	if (rc != EOK)
		return rc;

	rc = icp_add_fun(dev, "kbd", "arm/pl050", &icp_kbd_fun_proto);
	if (rc != EOK)
		return rc;

	rc = icp_add_fun(dev, "mouse", "arm/pl050", &icp_mouse_fun_proto);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Add device. */
static errno_t icp_dev_add(ddf_dev_t *dev)
{
	ddf_msg(LVL_NOTE, "icp_dev_add, device handle = %d",
	    (int)ddf_dev_get_handle(dev));

	/* Register functions. */
	if (icp_add_functions(dev)) {
		ddf_msg(LVL_ERROR, "Failed to add functions for ICP platform.");
	}

	return EOK;
}

int main(int argc, char *argv[])
{
	errno_t rc;

	printf(NAME ": HelenOS IntegratorCP platform driver\n");

	rc = ddf_log_init(NAME);
	if (rc != EOK) {
		printf(NAME ": Failed initializing logging service");
		return 1;
	}

	return ddf_driver_main(&icp_driver);
}

/**
 * @}
 */
