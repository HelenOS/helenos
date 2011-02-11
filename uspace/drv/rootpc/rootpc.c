/*
 * Copyright (c) 2010 Lenka Trochtova
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
 * @defgroup root_pc PC platform driver.
 * @brief HelenOS PC platform driver.
 * @{
 */

/** @file
 */

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <bool.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <str.h>
#include <ctype.h>
#include <macros.h>

#include <driver.h>
#include <devman.h>
#include <ipc/devman.h>
#include <ipc/dev_iface.h>
#include <ops/hw_res.h>
#include <device/hw_res.h>

#define NAME "rootpc"

typedef struct rootpc_fun_data {
	hw_resource_list_t hw_resources;
} rootpc_fun_data_t;

static int rootpc_add_device(device_t *dev);
static void root_pc_init(void);

/** The root device driver's standard operations. */
static driver_ops_t rootpc_ops = {
	.add_device = &rootpc_add_device
};

/** The root device driver structure. */
static driver_t rootpc_driver = {
	.name = NAME,
	.driver_ops = &rootpc_ops
};

static hw_resource_t pci_conf_regs = {
	.type = IO_RANGE,
	.res.io_range = {
		.address = 0xCF8,
		.size = 8,
		.endianness = LITTLE_ENDIAN
	}
};

static rootpc_fun_data_t pci_data = {
	.hw_resources = {
		1,
		&pci_conf_regs
	}
};

static hw_resource_list_t *rootpc_get_fun_resources(function_t *fun)
{
	rootpc_fun_data_t *data;
	
	data = (rootpc_fun_data_t *) fun->driver_data;
	if (NULL == data)
		return NULL;
	
	return &data->hw_resources;
}

static bool rootpc_enable_fun_interrupt(function_t *fun)
{
	/* TODO */
	
	return false;
}

static hw_res_ops_t fun_hw_res_ops = {
	&rootpc_get_fun_resources,
	&rootpc_enable_fun_interrupt
};

/* Initialized in root_pc_init() function. */
static device_ops_t rootpc_fun_ops;

static bool
rootpc_add_fun(device_t *parent, const char *name, const char *str_match_id,
    rootpc_fun_data_t *drv_data)
{
	printf(NAME ": adding new function '%s'.\n", name);
	
	function_t *fun = NULL;
	match_id_t *match_id = NULL;
	
	/* Create new device. */
	fun = create_function();
	if (fun == NULL)
		goto failure;
	
	fun->name = name;
	fun->driver_data = drv_data;
	fun->ftype = fun_inner;
	
	/* Initialize match id list */
	match_id = create_match_id();
	if (NULL == match_id)
		goto failure;
	
	match_id->id = str_match_id;
	match_id->score = 100;
	add_match_id(&fun->match_ids, match_id);
	
	/* Set provided operations to the device. */
	fun->ops = &rootpc_fun_ops;
	
	/* Register function. */
	if (EOK != register_function(fun, parent))
		goto failure;
	printf(NAME ": registered function handle = %u\n", fun->handle);
	
	return true;
	
failure:
	if (NULL != match_id)
		match_id->id = NULL;
	
	if (NULL != fun) {
		fun->name = NULL;
		delete_function(fun);
	}
	
	printf(NAME ": failed to add function '%s'.\n", name);
	
	return false;
}

static bool rootpc_add_functions(device_t *dev)
{
	return rootpc_add_fun(dev, "pci0", "intel_pci", &pci_data);
}

/** Get the root device.
 *
 * @param dev		The device which is root of the whole device tree (both
 *			of HW and pseudo devices).
 * @return		Zero on success, negative error number otherwise.
 */
static int rootpc_add_device(device_t *dev)
{
	printf(NAME ": rootpc_add_device, device handle = %d\n",
	    (int)dev->handle);
	
	/* Register functions. */
	if (!rootpc_add_functions(dev)) {
		printf(NAME ": failed to add functions for PC platform.\n");
	}
	
	return EOK;
}

static void root_pc_init(void)
{
	rootpc_fun_ops.interfaces[HW_RES_DEV_IFACE] = &fun_hw_res_ops;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS PC platform driver\n");
	root_pc_init();
	return driver_main(&rootpc_driver);
}

/**
 * @}
 */
