/*
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
 * @addtogroup sun4v sun4v
 * @{
 */

/** @file
 */

#include <as.h>
#include <assert.h>
#include <ddf/driver.h>
#include <ddf/log.h>
#include <errno.h>
#include <str_error.h>
#include <ops/hw_res.h>
#include <ops/pio_window.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysinfo.h>

#define NAME "sun4v"

typedef struct sun4v_fun {
	hw_resource_list_t hw_resources;
	pio_window_t pio_window;
} sun4v_fun_t;

static errno_t sun4v_dev_add(ddf_dev_t *dev);

static driver_ops_t sun4v_ops = {
	.dev_add = &sun4v_dev_add
};

static driver_t sun4v_driver = {
	.name = NAME,
	.driver_ops = &sun4v_ops
};

static hw_resource_t console_res[] = {
	{
		.type = MEM_RANGE,
		.res.mem_range = {
			.address = 0,
			.size = PAGE_SIZE,
			.relative = true,
			.endianness = LITTLE_ENDIAN
		}
	},
	{
		.type = MEM_RANGE,
		.res.mem_range = {
			.address = 0,
			.size = PAGE_SIZE,
			.relative = true,
			.endianness = LITTLE_ENDIAN
		}
	},
};

static sun4v_fun_t console_data = {
	.hw_resources = {
		sizeof(console_res) / sizeof(console_res[0]),
		console_res
	},
	.pio_window = {
		.mem = {
			.base = 0,
			.size = PAGE_SIZE
		}
	}
};

/** Obtain function soft-state from DDF function node */
static sun4v_fun_t *sun4v_fun(ddf_fun_t *fnode)
{
	return ddf_fun_data_get(fnode);
}

static hw_resource_list_t *sun4v_get_resources(ddf_fun_t *fnode)
{
	sun4v_fun_t *fun = sun4v_fun(fnode);

	assert(fun != NULL);
	return &fun->hw_resources;
}

static errno_t sun4v_enable_interrupt(ddf_fun_t *fun, int irq)
{
	return EOK;
}

static pio_window_t *sun4v_get_pio_window(ddf_fun_t *fnode)
{
	sun4v_fun_t *fun = sun4v_fun(fnode);

	assert(fun != NULL);
	return &fun->pio_window;
}

static hw_res_ops_t fun_hw_res_ops = {
	.get_resource_list = &sun4v_get_resources,
	.enable_interrupt = &sun4v_enable_interrupt,
};

static pio_window_ops_t fun_pio_window_ops = {
	.get_pio_window = &sun4v_get_pio_window
};

static ddf_dev_ops_t sun4v_fun_ops;

static errno_t sun4v_add_fun(ddf_dev_t *dev, const char *name,
    const char *str_match_id, sun4v_fun_t *fun_proto)
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

	sun4v_fun_t *fun = ddf_fun_data_alloc(fnode, sizeof(sun4v_fun_t));
	if (fun == NULL) {
		rc = ENOMEM;
		goto error;
	}

	*fun = *fun_proto;

	/* Add match ID */
	rc = ddf_fun_add_match_id(fnode, str_match_id, 100);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Error adding match ID");
		goto error;
	}

	/* Set provided operations to the device. */
	ddf_fun_set_ops(fnode, &sun4v_fun_ops);

	/* Register function. */
	rc = ddf_fun_bind(fnode);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function %s.", name);
		goto error;
	}

	return EOK;

error:
	if (fnode != NULL)
		ddf_fun_destroy(fnode);

	return rc;
}

static errno_t sun4v_add_functions(ddf_dev_t *dev)
{
	errno_t rc;

	rc = sun4v_add_fun(dev, "console", "sun4v/console", &console_data);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Add device. */
static errno_t sun4v_dev_add(ddf_dev_t *dev)
{
	ddf_msg(LVL_DEBUG, "sun4v_dev_add, device handle = %d",
	    (int)ddf_dev_get_handle(dev));

	/* Register functions. */
	if (sun4v_add_functions(dev)) {
		ddf_msg(LVL_ERROR, "Failed to add functions for sun4v platform.");
	}

	return EOK;
}

static errno_t sun4v_init(void)
{
	errno_t rc;
	sysarg_t paddr;

	sun4v_fun_ops.interfaces[HW_RES_DEV_IFACE] = &fun_hw_res_ops;
	sun4v_fun_ops.interfaces[PIO_WINDOW_DEV_IFACE] = &fun_pio_window_ops;

	rc = ddf_log_init(NAME);
	if (rc != EOK) {
		printf(NAME ": Failed initializing logging service\n");
		return rc;
	}

	rc = sysinfo_get_value("niagara.inbuf.address", &paddr);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "niagara.inbuf.address not set: %s", str_error(rc));
		return rc;
	}

	console_res[0].res.mem_range.address = paddr;

	rc = sysinfo_get_value("niagara.outbuf.address", &paddr);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "niagara.outbuf.address not set: %s", str_error(rc));
		return rc;
	}

	console_res[1].res.mem_range.address = paddr;
	return EOK;
}

int main(int argc, char *argv[])
{
	errno_t rc;

	printf(NAME ": Sun4v platform driver\n");

	rc = sun4v_init();
	if (rc != EOK)
		return 1;

	return ddf_driver_main(&sun4v_driver);
}

/**
 * @}
 */
