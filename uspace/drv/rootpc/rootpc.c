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
 * @defgroup root_pc Root HW device driver for ia32 and amd64 platform.
 * @brief HelenOS root HW device driver for ia32 and amd64 platform.
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
#include <resource.h>
#include <device/hw_res.h>

#define NAME "rootpc"

typedef struct rootpc_child_dev_data {
	hw_resource_list_t hw_resources;
} rootpc_child_dev_data_t;

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

static rootpc_child_dev_data_t pci_data = {
	.hw_resources = {
		1,
		&pci_conf_regs
	}
};

static hw_resource_list_t *rootpc_get_child_resources(device_t *dev)
{
	rootpc_child_dev_data_t *data;
	
	data = (rootpc_child_dev_data_t *) dev->driver_data;
	if (NULL == data)
		return NULL;
	
	return &data->hw_resources;
}

static bool rootpc_enable_child_interrupt(device_t *dev)
{
	/* TODO */
	
	return false;
}

static resource_iface_t child_res_iface = {
	&rootpc_get_child_resources,
	&rootpc_enable_child_interrupt
};

/* Initialized in root_pc_init() function. */
static device_ops_t rootpc_child_ops;

static bool
rootpc_add_child(device_t *parent, const char *name, const char *str_match_id,
    rootpc_child_dev_data_t *drv_data)
{
	printf(NAME ": adding new child device '%s'.\n", name);
	
	device_t *child = NULL;
	match_id_t *match_id = NULL;
	
	/* Create new device. */
	child = create_device();
	if (NULL == child)
		goto failure;
	
	child->name = name;
	child->driver_data = drv_data;
	
	/* Initialize match id list */
	match_id = create_match_id();
	if (NULL == match_id)
		goto failure;
	
	match_id->id = str_match_id;
	match_id->score = 100;
	add_match_id(&child->match_ids, match_id);
	
	/* Set provided operations to the device. */
	child->ops = &rootpc_child_ops;
	
	/* Register child device. */
	if (EOK != child_device_register(child, parent))
		goto failure;
	
	return true;
	
failure:
	if (NULL != match_id)
		match_id->id = NULL;
	
	if (NULL != child) {
		child->name = NULL;
		delete_device(child);
	}
	
	printf(NAME ": failed to add child device '%s'.\n", name);
	
	return false;
}

static bool rootpc_add_children(device_t *dev)
{
	return rootpc_add_child(dev, "pci0", "intel_pci", &pci_data);
}

/** Get the root device.
 *
 * @param dev		The device which is root of the whole device tree (both
 *			of HW and pseudo devices).
 * @return		Zero on success, negative error number otherwise.
 */
static int rootpc_add_device(device_t *dev)
{
	printf(NAME ": rootpc_add_device, device handle = %d\n", dev->handle);
	
	/* Register child devices. */
	if (!rootpc_add_children(dev)) {
		printf(NAME ": failed to add child devices for platform "
		    "ia32.\n");
	}
	
	return EOK;
}

static void root_pc_init(void)
{
	rootpc_child_ops.interfaces[HW_RES_DEV_IFACE] = &child_res_iface;
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS rootpc device driver\n");
	root_pc_init();
	return driver_main(&rootpc_driver);
}

/**
 * @}
 */
