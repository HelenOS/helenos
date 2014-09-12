/*
 * Copyright (c) 2011 Martin Decky
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
 * @defgroup mac Mac platform driver.
 * @brief HelenOS Mac platform driver.
 * @{
 */

/** @file
 */

#include <ddf/driver.h>
#include <ddf/log.h>
#include <errno.h>
#include <ops/hw_res.h>
#include <stdio.h>

#define NAME  "mac"

typedef struct {
	hw_resource_list_t hw_resources;
} mac_fun_t;

static hw_resource_t pci_conf_regs[] = {
	{
		.type = IO_RANGE,
		.res.io_range = {
			.address = 0xfec00000,
			.size = 4,
			.relative = false,
			.endianness = LITTLE_ENDIAN
		}
	},
	{
		.type = IO_RANGE,
		.res.io_range = {
			.address = 0xfee00000,
			.size = 4,
			.relative = false,
			.endianness = LITTLE_ENDIAN
		}
	}
};

static mac_fun_t pci_data = {
	.hw_resources = {
		2,
		pci_conf_regs
	}
};

static ddf_dev_ops_t mac_fun_ops;

/** Obtain function soft-state from DDF function node */
static mac_fun_t *mac_fun(ddf_fun_t *fnode)
{
	return ddf_fun_data_get(fnode);
}

static bool mac_add_fun(ddf_dev_t *dev, const char *name,
    const char *str_match_id, mac_fun_t *fun_proto)
{
	ddf_msg(LVL_DEBUG, "Adding new function '%s'.", name);
	
	ddf_fun_t *fnode = NULL;
	int rc;
	
	/* Create new device. */
	fnode = ddf_fun_create(dev, fun_inner, name);
	if (fnode == NULL)
		goto failure;
	
	mac_fun_t *fun = ddf_fun_data_alloc(fnode, sizeof(mac_fun_t));
	*fun = *fun_proto;
	
	/* Add match ID */
	rc = ddf_fun_add_match_id(fnode, str_match_id, 100);
	if (rc != EOK)
		goto failure;
	
	/* Set provided operations to the device. */
	ddf_fun_set_ops(fnode, &mac_fun_ops);
	
	/* Register function. */
	if (ddf_fun_bind(fnode) != EOK) {
		ddf_msg(LVL_ERROR, "Failed binding function %s.", name);
		goto failure;
	}
	
	return true;
	
failure:
	if (fnode != NULL)
		ddf_fun_destroy(fnode);
	
	ddf_msg(LVL_ERROR, "Failed adding function '%s'.", name);
	
	return false;
}

/** Get the root device.
 *
 * @param dev Device which is root of the whole device tree
 *            (both of HW and pseudo devices).
 *
 * @return Zero on success, negative error number otherwise.
 *
 */
static int mac_dev_add(ddf_dev_t *dev)
{
#if 0
	/* Register functions */
	if (!mac_add_fun(dev, "pci0", "intel_pci", &pci_data))
		ddf_msg(LVL_ERROR, "Failed to add functions for Mac platform.");
#else
	(void)pci_data;
	(void)mac_add_fun;
#endif
	
	return EOK;
}

/** The root device driver's standard operations. */
static driver_ops_t mac_ops = {
	.dev_add = &mac_dev_add
};

/** The root device driver structure. */
static driver_t mac_driver = {
	.name = NAME,
	.driver_ops = &mac_ops
};

static hw_resource_list_t *mac_get_resources(ddf_fun_t *fnode)
{
	mac_fun_t *fun = mac_fun(fnode);
	assert(fun != NULL);
	
	return &fun->hw_resources;
}

static bool mac_enable_interrupt(ddf_fun_t *fun)
{
	/* TODO */
	
	return false;
}

static hw_res_ops_t fun_hw_res_ops = {
   	.get_resource_list = &mac_get_resources,
	.enable_interrupt = &mac_enable_interrupt
};

int main(int argc, char *argv[])
{
	printf("%s: HelenOS Mac platform driver\n", NAME);
	ddf_log_init(NAME);
	mac_fun_ops.interfaces[HW_RES_DEV_IFACE] = &fun_hw_res_ops;
	return ddf_driver_main(&mac_driver);
}

/**
 * @}
 */
