/*
 * Copyright (c) 2012 Jan Vesely
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
 * @defgroup amdm37x TI AM/DM37x platform driver.
 * @brief HelenOS TI AM/DM37x platform driver.
 * @{
 */

/** @file
 */

#define DEBUG_CM 0

#include <ddf/log.h>
#include <errno.h>
#include <ops/hw_res.h>
#include <stdio.h>

#include "amdm37x.h"

#define NAME  "amdm37x"

typedef struct {
	const char *name;
	const char *id;
	int score;
	hw_resource_list_t hw_resources;
} amdm37x_fun_t;

/* See amdm37x TRM page 3316 for these values */
#define OHCI_BASE_ADDRESS   0x48064400
#define OHCI_SIZE   1024
#define EHCI_BASE_ADDRESS   0x48064800
#define EHCI_SIZE   1024

/* See amdm37x TRM page 1813 for these values */
#define DSS_BASE_ADDRESS   0x48050000
#define DSS_SIZE   512
#define DISPC_BASE_ADDRESS   0x48050400
#define DISPC_SIZE   1024
#define VIDEO_ENC_BASE_ADDRESS   0x48050C00
#define VIDEO_ENC_SIZE   256


static hw_resource_t ohci_res[] = {
	{
		.type = MEM_RANGE,
		.res.io_range = {
			.address = OHCI_BASE_ADDRESS,
			.size = OHCI_SIZE,
			.endianness = LITTLE_ENDIAN
		},
	},
	{
		.type = INTERRUPT,
		.res.interrupt = { .irq = 76 },
	},
};

static hw_resource_t ehci_res[] = {
	{
		.type = MEM_RANGE,
		/* See amdm37x TRM page. 3316 for these values */
		.res.io_range = {
			.address = EHCI_BASE_ADDRESS,
			.size = EHCI_SIZE,
			.endianness = LITTLE_ENDIAN
		},
	},
	{
		.type = INTERRUPT,
		.res.interrupt = { .irq = 77 },
	},
};

static hw_resource_t disp_res[] = {
	{
		.type = MEM_RANGE,
		.res.io_range = {
			.address = DSS_BASE_ADDRESS,
			.size = DSS_SIZE,
			.endianness = LITTLE_ENDIAN
		},
	},
	{
		.type = MEM_RANGE,
		.res.io_range = {
			.address = DISPC_BASE_ADDRESS,
			.size = DISPC_SIZE,
			.endianness = LITTLE_ENDIAN
		},
	},
	{
		.type = MEM_RANGE,
		.res.io_range = {
			.address = VIDEO_ENC_BASE_ADDRESS,
			.size = VIDEO_ENC_SIZE,
			.endianness = LITTLE_ENDIAN
		},
	},
	{
		.type = INTERRUPT,
		.res.interrupt = { .irq = 25 },
	},
};

static const amdm37x_fun_t amdm37x_funcs[] = {
{
	.name = "ohci",
	.id = "usb/host=ohci",
	.score = 90,
	.hw_resources = { .resources = ohci_res, .count = ARRAY_SIZE(ohci_res) }
},
{
	.name = "ehci",
	.id = "usb/host=ehci",
	.score = 90,
	.hw_resources = { .resources = ehci_res, .count = ARRAY_SIZE(ehci_res) }
},
{
	.name = "fb",
	.id = "amdm37x&dispc",
	.score = 90,
	.hw_resources = { .resources = disp_res, .count = ARRAY_SIZE(disp_res) }
},
};


static hw_resource_list_t *amdm37x_get_resources(ddf_fun_t *fnode);
static errno_t amdm37x_enable_interrupt(ddf_fun_t *fun, int);

static hw_res_ops_t fun_hw_res_ops = {
	.get_resource_list = &amdm37x_get_resources,
	.enable_interrupt = &amdm37x_enable_interrupt,
};

static ddf_dev_ops_t amdm37x_fun_ops = {
	.interfaces[HW_RES_DEV_IFACE] = &fun_hw_res_ops
};

static errno_t amdm37x_add_fun(ddf_dev_t *dev, const amdm37x_fun_t *fun)
{
	assert(dev);
	assert(fun);

	ddf_msg(LVL_DEBUG, "Adding new function '%s'.", fun->name);

	/* Create new device function. */
	ddf_fun_t *fnode = ddf_fun_create(dev, fun_inner, fun->name);
	if (fnode == NULL)
		return ENOMEM;
	
	/* Add match id */
	errno_t ret = ddf_fun_add_match_id(fnode, fun->id, fun->score);
	if (ret != EOK) {
		ddf_fun_destroy(fnode);
		return ret;
	}
	
	/* Alloc needed data */
	amdm37x_fun_t *rf =
	    ddf_fun_data_alloc(fnode, sizeof(amdm37x_fun_t));
	if (!rf) {
		ddf_fun_destroy(fnode);
		return ENOMEM;
	}
	*rf = *fun;

	/* Set provided operations to the device. */
	ddf_fun_set_ops(fnode, &amdm37x_fun_ops);
	
	/* Register function. */
	ret = ddf_fun_bind(fnode);
	if (ret != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function %s.", fun->name);
		ddf_fun_destroy(fnode);
		return ret;
	}
	
	return EOK;
}

/** Add the root device.
 *
 * @param dev Device which is root of the whole device tree
 *            (both of HW and pseudo devices).
 *
 * @return Zero on success, error number otherwise.
 *
 */
static errno_t amdm37x_dev_add(ddf_dev_t *dev)
{
	assert(dev);
	amdm37x_t *device = ddf_dev_data_alloc(dev, sizeof(amdm37x_t));
	if (!device)
		return ENOMEM;
	errno_t ret = amdm37x_init(device, DEBUG_CM);
	if (ret != EOK) {
		ddf_msg(LVL_FATAL, "Failed to setup hw access!.\n");
		return ret;
	}

	/* Set dplls to ON and automatic */
	amdm37x_setup_dpll_on_autoidle(device);

	/* Enable function and interface clocks */
	amdm37x_usb_clocks_set(device, true);

	/* Init TLL */
	ret = amdm37x_usb_tll_init(device);
	if (ret != EOK) {
		ddf_msg(LVL_FATAL, "Failed to init USB TLL!.\n");
		amdm37x_usb_clocks_set(device, false);
		return ret;
	}

	/* Register functions */
	for (unsigned i = 0; i < ARRAY_SIZE(amdm37x_funcs); ++i) {
		if (amdm37x_add_fun(dev, &amdm37x_funcs[i]) != EOK)
			ddf_msg(LVL_ERROR, "Failed to add %s function for "
			    "BeagleBoard-xM platform.", amdm37x_funcs[i].name);
	}
	return EOK;
}

/** The root device driver's standard operations. */
static driver_ops_t amdm37x_ops = {
	.dev_add = &amdm37x_dev_add
};

/** The root device driver structure. */
static driver_t amdm37x_driver = {
	.name = NAME,
	.driver_ops = &amdm37x_ops
};

static hw_resource_list_t * amdm37x_get_resources(ddf_fun_t *fnode)
{
	amdm37x_fun_t *fun = ddf_fun_data_get(fnode);
	assert(fun != NULL);
	return &fun->hw_resources;
}

static errno_t amdm37x_enable_interrupt(ddf_fun_t *fun, int irq)
{
	//TODO: Implement
	return false;
}

int main(int argc, char *argv[])
{
	printf("%s: HelenOS AM/DM37x(OMAP37x) platform driver\n", NAME);
	ddf_log_init(NAME);
	return ddf_driver_main(&amdm37x_driver);
}

/**
 * @}
 */
