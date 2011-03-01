/*
 * Copyright (c) 2011 Vojtech Horky, Jan Vesely
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
/** @addtogroup usb
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#include <ddf/driver.h>
#include <devman.h>
#include <device/hw_res.h>
#include <usb_iface.h>
#include <usb/ddfiface.h>

#include <errno.h>

#include <usb/debug.h>

#include "root_hub.h"

#define NAME "uhci-rhd"
static int hc_get_my_registers(ddf_dev_t *dev,
    uintptr_t *io_reg_address, size_t *io_reg_size);

static int usb_iface_get_hc_handle(ddf_fun_t *fun, devman_handle_t *handle)
{
	assert(fun);
	assert(fun->driver_data);
	assert(handle);

	*handle = ((uhci_root_hub_t*)fun->driver_data)->hc_handle;

	return EOK;
}

static usb_iface_t uhci_rh_usb_iface = {
	.get_hc_handle = usb_iface_get_hc_handle,
	.get_address = usb_iface_get_address_hub_impl
};

static ddf_dev_ops_t uhci_rh_ops = {
	.interfaces[USB_DEV_IFACE] = &uhci_rh_usb_iface,
};

static int uhci_rh_add_device(ddf_dev_t *device)
{
	if (!device)
		return ENOTSUP;

	usb_log_debug2("%s called device %d\n", __FUNCTION__, device->handle);

	//device->ops = &uhci_rh_ops;
	(void) uhci_rh_ops;

	uhci_root_hub_t *rh = malloc(sizeof(uhci_root_hub_t));
	if (!rh) {
		usb_log_error("Failed to allocate memory for driver instance.\n");
		return ENOMEM;
	}

	uintptr_t io_regs = 0;
	size_t io_size = 0;

	int ret = hc_get_my_registers(device, &io_regs, &io_size);
	assert(ret == EOK);

	/* TODO: verify values from hc */
	usb_log_info("I/O regs at 0x%X (size %zu).\n", io_regs, io_size);
	ret = uhci_root_hub_init(rh, (void*)io_regs, io_size, device);
	if (ret != EOK) {
		usb_log_error("Failed(%d) to initialize driver instance.\n", ret);
		free(rh);
		return ret;
	}

	device->driver_data = rh;
	usb_log_info("Sucessfully initialized driver instance for device:%d.\n",
	    device->handle);
	return EOK;
}

static driver_ops_t uhci_rh_driver_ops = {
	.add_device = uhci_rh_add_device,
};

static driver_t uhci_rh_driver = {
	.name = NAME,
	.driver_ops = &uhci_rh_driver_ops
};
/*----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	usb_log_enable(USB_LOG_LEVEL_DEBUG, NAME);
	return ddf_driver_main(&uhci_rh_driver);
}
/*----------------------------------------------------------------------------*/
int hc_get_my_registers(ddf_dev_t *dev,
    uintptr_t *io_reg_address, size_t *io_reg_size)
{
	assert(dev != NULL);

	int parent_phone = devman_parent_device_connect(dev->handle,
	    IPC_FLAG_BLOCKING);
	if (parent_phone < 0) {
		return parent_phone;
	}

	int rc;

	hw_resource_list_t hw_resources;
	rc = hw_res_get_resource_list(parent_phone, &hw_resources);
	if (rc != EOK) {
		goto leave;
	}

	uintptr_t io_address = 0;
	size_t io_size = 0;
	bool io_found = false;

	size_t i;
	for (i = 0; i < hw_resources.count; i++) {
		hw_resource_t *res = &hw_resources.resources[i];
		switch (res->type) {
			case IO_RANGE:
				io_address = (uintptr_t)
				    res->res.io_range.address;
				io_size = res->res.io_range.size;
				io_found = true;
				break;
			default:
				break;
		}
	}

	if (!io_found) {
		rc = ENOENT;
		goto leave;
	}

	if (io_reg_address != NULL) {
		*io_reg_address = io_address;
	}
	if (io_reg_size != NULL) {
		*io_reg_size = io_size;
	}
	rc = EOK;
leave:
	async_hangup(parent_phone);

	return rc;
}
/**
 * @}
 */
