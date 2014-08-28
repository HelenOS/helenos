/*
 * Copyright (c) 2012 Jan Vesely
 * Copyright (c) 2013 Jakub Klama
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
 * @defgroup leon3 SPARC LEON3 platform driver.
 * @brief HelenOS SPARC LEON3 platform driver.
 * @{
 */
/** @file
 */

#include <ddf/log.h>
#include <errno.h>
#include <ops/hw_res.h>
#include <stdio.h>
#include "leon3.h"

#define NAME  "leon3"

typedef struct {
	const char *name;
	match_id_t match_id;
	hw_resource_list_t hw_resources;
} leon3_fun_t;

static hw_resource_t amba_res[] = {
	{
		.type = MEM_RANGE,
		.res.mem_range = {
			.address = AMBAPP_MASTER_AREA,
			.size = AMBAPP_MASTER_SIZE,
			.endianness = BIG_ENDIAN
		}
	},
	{
		.type = MEM_RANGE,
		.res.mem_range = {
			.address = AMBAPP_SLAVE_AREA,
			.size = AMBAPP_SLAVE_SIZE,
			.endianness = BIG_ENDIAN,
		}
	}
};

static const leon3_fun_t leon3_func = {
	.name = "leon_amba",
	.match_id = {
		.id =  "leon_amba",
		.score = 90 
	},
	.hw_resources = {
		.count = 2,
		.resources = amba_res
	}
};

static hw_resource_list_t *leon3_get_resources(ddf_fun_t *);
static bool leon3_enable_interrupt(ddf_fun_t *);

static hw_res_ops_t fun_hw_res_ops = {
	.get_resource_list = &leon3_get_resources,
	.enable_interrupt = &leon3_enable_interrupt
};

static ddf_dev_ops_t leon3_fun_ops = {
	.interfaces[HW_RES_DEV_IFACE] = &fun_hw_res_ops
};

static int leon3_add_fun(ddf_dev_t *dev, const leon3_fun_t *fun)
{
	assert(dev);
	assert(fun);
	
	ddf_msg(LVL_DEBUG, "Adding new function '%s'.", fun->name);
	
	/* Create new device function. */
	ddf_fun_t *fnode = ddf_fun_create(dev, fun_inner, fun->name);
	if (fnode == NULL)
		return ENOMEM;
	
	/* Add match id */
	int ret = ddf_fun_add_match_id(fnode, fun->match_id.id,
	    fun->match_id.score);
	if (ret != EOK) {
		ddf_fun_destroy(fnode);
		return ret;
	}
	
	/* Allocate needed data */
	leon3_fun_t *rf =
	    ddf_fun_data_alloc(fnode, sizeof(leon3_fun_t));
	if (!rf) {
		ddf_fun_destroy(fnode);
		return ENOMEM;
	}
	*rf = *fun;
	
	/* Set provided operations to the device. */
	ddf_fun_set_ops(fnode, &leon3_fun_ops);
	
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
 * @return Zero on success, negative error number otherwise.
 *
 */
static int leon3_dev_add(ddf_dev_t *dev)
{
	assert(dev);
	
	/* Register functions */
	if (leon3_add_fun(dev, &leon3_func) != EOK) {
		ddf_msg(LVL_ERROR, "Failed to add %s function for "
		    "LEON3 platform.", leon3_func.name);
	}
	
	return EOK;
}

/** The root device driver's standard operations. */
static driver_ops_t leon3_ops = {
	.dev_add = &leon3_dev_add
};

/** The root device driver structure. */
static driver_t leon3_driver = {
	.name = NAME,
	.driver_ops = &leon3_ops
};

static hw_resource_list_t *leon3_get_resources(ddf_fun_t *fnode)
{
	leon3_fun_t *fun = ddf_fun_data_get(fnode);
	assert(fun != NULL);
	
	printf("leon3_get_resources() called\n");
	
	return &fun->hw_resources;
}

static bool leon3_enable_interrupt(ddf_fun_t *fun)
{
	// FIXME TODO
	return false;
}

int main(int argc, char *argv[])
{
	printf("%s: HelenOS SPARC LEON3 platform driver\n", NAME);
	ddf_log_init(NAME);
	return ddf_driver_main(&leon3_driver);
}

/**
 * @}
 */
