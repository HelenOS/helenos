/*
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup drvusbuhci
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#include <assert.h>
#include <errno.h>
#include <str_error.h>
#include <stdio.h>
#include <ops/hw_res.h>

#include <usb_iface.h>
#include <usb/debug.h>

#include "root_hub.h"
/** DDF support structure for uhci_rhd driver, provides I/O resources */
typedef struct {
	/** List of resources available to the root hub. */
	hw_resource_list_t resource_list;
	/** The only resource in the RH resource list */
	hw_resource_t io_regs;

	devman_handle_t hc_handle;
} rh_t;

/** Gets handle of the respective hc.
 *
 * @param[in] fun DDF function of uhci device.
 * @param[out] handle Host cotnroller handle.
 * @return Error code.
 */
static int usb_iface_get_hc_handle(ddf_fun_t *fun, devman_handle_t *handle)
{
	assert(fun);
	rh_t *rh = ddf_fun_data_get(fun);

	if (handle != NULL)
		*handle = rh->hc_handle;
	return EOK;
}

/** USB interface implementation used by RH */
static usb_iface_t usb_iface = {
	.get_hc_handle = usb_iface_get_hc_handle,
};

/** Get root hub hw resources (I/O registers).
 *
 * @param[in] fun Root hub function.
 * @return Pointer to the resource list used by the root hub.
 */
static hw_resource_list_t *get_resource_list(ddf_fun_t *fun)
{
	assert(fun);
	rh_t *rh = ddf_fun_data_get(fun);
	assert(rh);
	return &rh->resource_list;
}

/** Interface to provide the root hub driver with hw info */
static hw_res_ops_t hw_res_iface = {
	.get_resource_list = get_resource_list,
	.enable_interrupt = NULL,
};

/** RH function support for uhci_rhd */
static ddf_dev_ops_t rh_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface,
	.interfaces[HW_RES_DEV_IFACE] = &hw_res_iface
};
/** Root hub initialization
 * @param[in] instance RH structure to initialize
 * @param[in] fun DDF function representing UHCI root hub
 * @param[in] reg_addr Address of root hub status and control registers.
 * @param[in] reg_size Size of accessible address space.
 * @return Error code.
 */
int rh_init(ddf_dev_t *device, uintptr_t reg_addr, size_t reg_size,
    devman_handle_t handle)
{
	assert(device);
	
	ddf_fun_t *fun = ddf_fun_create(device, fun_inner, "uhci_rh");
	if (!fun) {
		usb_log_error("Failed to create UHCI RH function.\n");
		return ENOMEM;
	}
	ddf_fun_set_ops(fun, &rh_ops);

	rh_t *instance = ddf_fun_data_alloc(fun, sizeof(rh_t));

	/* Initialize resource structure */
	instance->resource_list.count = 1;
	instance->resource_list.resources = &instance->io_regs;

	instance->io_regs.type = IO_RANGE;
	instance->io_regs.res.io_range.address = reg_addr;
	instance->io_regs.res.io_range.size = reg_size;
	instance->io_regs.res.io_range.endianness = LITTLE_ENDIAN;

	instance->hc_handle = handle;	

	int ret = ddf_fun_add_match_id(fun, "usb&uhci&root-hub", 100);
	if (ret != EOK) {
		usb_log_error("Failed to add root hub match id: %s\n",
		    str_error(ret));
		ddf_fun_destroy(fun);
	}

	ret = ddf_fun_bind(fun);
	if (ret != EOK) {
		usb_log_error("Failed to bind root function: %s\n",
		    str_error(ret));
		ddf_fun_destroy(fun);
	}

	usb_log_fatal("RH ready, hc handle: %"PRIun"\n", handle);
	return ret;
}
/**
 * @}
 */
