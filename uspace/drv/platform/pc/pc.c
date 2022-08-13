/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @addtogroup pc pc
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

#include <ddf/driver.h>
#include <ddf/log.h>
#include <ipc/dev_iface.h>
#include <ops/hw_res.h>
#include <ops/pio_window.h>

#define NAME "pc"

typedef struct pc_fun {
	hw_resource_list_t hw_resources;
	pio_window_t pio_window;
} pc_fun_t;

static errno_t pc_dev_add(ddf_dev_t *dev);
static void pc_init(void);

/** The root device driver's standard operations. */
static driver_ops_t pc_ops = {
	.dev_add = &pc_dev_add
};

/** The root device driver structure. */
static driver_t pc_driver = {
	.name = NAME,
	.driver_ops = &pc_ops
};

static hw_resource_t pci_conf_regs[] = {
	{
		.type = IO_RANGE,
		.res.io_range = {
			.address = 0xCF8,
			.size = 4,
			.relative = false,
			.endianness = LITTLE_ENDIAN
		}
	},
	{
		.type = IO_RANGE,
		.res.io_range = {
			.address = 0xCFC,
			.size = 4,
			.relative = false,
			.endianness = LITTLE_ENDIAN
		}
	}
};

static pc_fun_t sys_data = {
	.hw_resources = {
		sizeof(pci_conf_regs) / sizeof(pci_conf_regs[0]),
		pci_conf_regs
	},
	.pio_window = {
		.mem = {
			.base = UINT32_C(0),
			.size = UINT32_C(0xffffffff) /* practical maximum */
		},
		.io = {
			.base = UINT32_C(0),
			.size = UINT32_C(0x10000)
		}
	}
};

/** Obtain function soft-state from DDF function node */
static pc_fun_t *pc_fun(ddf_fun_t *fnode)
{
	return ddf_fun_data_get(fnode);
}

static hw_resource_list_t *pc_get_resources(ddf_fun_t *fnode)
{
	pc_fun_t *fun = pc_fun(fnode);

	assert(fun != NULL);
	return &fun->hw_resources;
}

static errno_t pc_enable_interrupt(ddf_fun_t *fun, int irq)
{
	/* TODO */

	return false;
}

static pio_window_t *pc_get_pio_window(ddf_fun_t *fnode)
{
	pc_fun_t *fun = pc_fun(fnode);

	assert(fun != NULL);
	return &fun->pio_window;
}

static hw_res_ops_t fun_hw_res_ops = {
	.get_resource_list = &pc_get_resources,
	.enable_interrupt = &pc_enable_interrupt,
};

static pio_window_ops_t fun_pio_window_ops = {
	.get_pio_window = &pc_get_pio_window
};

/* Initialized in pc_init() function. */
static ddf_dev_ops_t pc_fun_ops;

static errno_t pc_add_sysbus(ddf_dev_t *dev)
{
	ddf_fun_t *fnode = NULL;
	errno_t rc;

	ddf_msg(LVL_DEBUG, "Adding system bus.");

	/* Create new device. */
	fnode = ddf_fun_create(dev, fun_inner, "sys");
	if (fnode == NULL) {
		rc = ENOMEM;
		goto error;
	}

	pc_fun_t *fun = ddf_fun_data_alloc(fnode, sizeof(pc_fun_t));
	*fun = sys_data;

	/* Add match IDs */
	rc = ddf_fun_add_match_id(fnode, "intel_pci", 100);
	if (rc != EOK)
		goto error;

	rc = ddf_fun_add_match_id(fnode, "isa", 10);
	if (rc != EOK)
		goto error;

	/* Set provided operations to the device. */
	ddf_fun_set_ops(fnode, &pc_fun_ops);

	/* Register function. */
	rc = ddf_fun_bind(fnode);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding system bus function.");
		goto error;
	}

	return EOK;

error:
	if (fnode != NULL)
		ddf_fun_destroy(fnode);

	ddf_msg(LVL_ERROR, "Failed adding system bus.");
	return rc;
}

static errno_t pc_add_functions(ddf_dev_t *dev)
{
	errno_t rc;

	rc = pc_add_sysbus(dev);
	if (rc != EOK)
		return rc;

	return EOK;
}

/** Get the root device.
 *
 * @param dev		The device which is root of the whole device tree (both
 *			of HW and pseudo devices).
 * @return		Zero on success, error number otherwise.
 */
static errno_t pc_dev_add(ddf_dev_t *dev)
{
	errno_t rc;

	ddf_msg(LVL_DEBUG, "pc_dev_add, device handle = %d",
	    (int)ddf_dev_get_handle(dev));

	/* Register functions. */
	rc = pc_add_functions(dev);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to add functions for PC platform.");
		return rc;
	}

	return EOK;
}

static void pc_init(void)
{
	ddf_log_init(NAME);
	pc_fun_ops.interfaces[HW_RES_DEV_IFACE] = &fun_hw_res_ops;
	pc_fun_ops.interfaces[PIO_WINDOW_DEV_IFACE] = &fun_pio_window_ops;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS PC platform driver\n");
	pc_init();
	return ddf_driver_main(&pc_driver);
}

/**
 * @}
 */
