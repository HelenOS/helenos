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
 * @defgroup root_amdm37x TI AM/DM37x platform driver.
 * @brief HelenOS TI AM/DM37x platform driver.
 * @{
 */

/** @file
 */
#define _DDF_DATA_IMPLANT

#define DEBUG_CM 0

#include <ddf/log.h>
#include <errno.h>
#include <ops/hw_res.h>
#include <stdio.h>

#include "amdm37x.h"

#define NAME  "rootamdm37x"

typedef struct {
	hw_resource_list_t hw_resources;
} rootamdm37x_fun_t;

#define OHCI_BASE_ADDRESS  0x48064400
#define OHCI_SIZE  1024
#define EHCI_BASE_ADDRESS  0x48064800
#define EHCI_SIZE  1024

static hw_resource_t ohci_res[] = {
	{
		.type = MEM_RANGE,
		/* See amdm37x TRM page. 3316 for these values */
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

static const rootamdm37x_fun_t ohci = {
	.hw_resources = {
	    .resources = ohci_res,
	    .count = sizeof(ohci_res)/sizeof(ohci_res[0]),
	}
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

static const rootamdm37x_fun_t ehci = {
	.hw_resources = {
	    .resources = ehci_res,
	    .count = sizeof(ehci_res) / sizeof(ehci_res[0]),
	}
};

static hw_resource_list_t *rootamdm37x_get_resources(ddf_fun_t *fnode);
static bool rootamdm37x_enable_interrupt(ddf_fun_t *fun);

static hw_res_ops_t fun_hw_res_ops = {
	.get_resource_list = &rootamdm37x_get_resources,
	.enable_interrupt = &rootamdm37x_enable_interrupt,
};

static ddf_dev_ops_t rootamdm37x_fun_ops =
{
	.interfaces[HW_RES_DEV_IFACE] = &fun_hw_res_ops
};

static int rootamdm37x_add_fun(ddf_dev_t *dev, const char *name,
    const char *str_match_id, const rootamdm37x_fun_t *fun)
{
	ddf_msg(LVL_DEBUG, "Adding new function '%s'.", name);
	
	/* Create new device function. */
	ddf_fun_t *fnode = ddf_fun_create(dev, fun_inner, name);
	if (fnode == NULL)
		return ENOMEM;
	
	
	/* Add match id */
	int ret = ddf_fun_add_match_id(fnode, str_match_id, 100);
	if (ret != EOK) {
		ddf_fun_destroy(fnode);
		return ret;
	}
	
	/* Set provided operations to the device. */
	ddf_fun_data_implant(fnode, (void*)fun);
	ddf_fun_set_ops(fnode, &rootamdm37x_fun_ops);
	
	/* Register function. */
	ret = ddf_fun_bind(fnode);
	if (ret != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function %s.", name);
		// TODO This will try to free our data!
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
 * @return Zero on success, negative error number otherwise.
 *
 */
static int rootamdm37x_dev_add(ddf_dev_t *dev)
{
	assert(dev);
	amdm37x_t *device = ddf_dev_data_alloc(dev, sizeof(amdm37x_t));
	if (!device)
		return ENOMEM;
	int ret = amdm37x_init(device, DEBUG_CM);
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
	if (rootamdm37x_add_fun(dev, "ohci", "usb/host=ohci", &ohci) != EOK)
		ddf_msg(LVL_ERROR, "Failed to add OHCI function for "
		    "BeagleBoard-xM platform.");
	if (rootamdm37x_add_fun(dev, "ehci", "usb/host=ehci", &ehci) != EOK)
		ddf_msg(LVL_ERROR, "Failed to add EHCI function for "
		    "BeagleBoard-xM platform.");

	return EOK;
}

/** The root device driver's standard operations. */
static driver_ops_t rootamdm37x_ops = {
	.dev_add = &rootamdm37x_dev_add
};

/** The root device driver structure. */
static driver_t rootamdm37x_driver = {
	.name = NAME,
	.driver_ops = &rootamdm37x_ops
};

static hw_resource_list_t * rootamdm37x_get_resources(ddf_fun_t *fnode)
{
	rootamdm37x_fun_t *fun = ddf_fun_data_get(fnode);
	assert(fun != NULL);
	return &fun->hw_resources;
}

static bool rootamdm37x_enable_interrupt(ddf_fun_t *fun)
{
	//TODO: Implement
	return false;
}

int main(int argc, char *argv[])
{
	printf("%s: HelenOS AM/DM37x(OMAP37x) platform driver\n", NAME);
	ddf_log_init(NAME);
	return ddf_driver_main(&rootamdm37x_driver);
}

/**
 * @}
 */
