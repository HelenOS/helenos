/*
 * Copyright (c) 2011 Vojtech Horky
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
 * @addtogroup drvusbuhci
 * @{
 */
/**
 * @file
 * PCI related functions needed by the UHCI driver.
 */
#include <errno.h>
#include <assert.h>
#include <devman.h>
#include <device/hw_res.h>

#include <usb/debug.h>
#include <pci_dev_iface.h>

#include "pci.h"

/** Get address of registers and IRQ for given device.
 *
 * @param[in] dev Device asking for the addresses.
 * @param[out] io_reg_address Base address of the I/O range.
 * @param[out] io_reg_size Size of the I/O range.
 * @param[out] irq_no IRQ assigned to the device.
 * @return Error code.
 */
int pci_get_my_registers(ddf_dev_t *dev,
    uintptr_t *io_reg_address, size_t *io_reg_size,
    int *irq_no)
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

	int irq = 0;
	bool irq_found = false;

	size_t i;
	for (i = 0; i < hw_resources.count; i++) {
		hw_resource_t *res = &hw_resources.resources[i];
		switch (res->type)
		{
		case INTERRUPT:
			irq = res->res.interrupt.irq;
			irq_found = true;
			usb_log_debug2("Found interrupt: %d.\n", irq);
			break;

		case IO_RANGE:
			io_address = res->res.io_range.address;
			io_size = res->res.io_range.size;
			usb_log_debug2("Found io: %llx %zu.\n",
			    res->res.io_range.address, res->res.io_range.size);
			io_found = true;

		default:
			break;
		}
	}

	if (!io_found || !irq_found) {
		rc = ENOENT;
		goto leave;
	}

	*io_reg_address = io_address;
	*io_reg_size = io_size;
	*irq_no = irq;

	rc = EOK;
leave:
	async_hangup(parent_phone);

	return rc;
}
/*----------------------------------------------------------------------------*/
int pci_enable_interrupts(ddf_dev_t *device)
{
	int parent_phone = devman_parent_device_connect(device->handle,
	    IPC_FLAG_BLOCKING);
	bool enabled = hw_res_enable_interrupt(parent_phone);
	async_hangup(parent_phone);
	return enabled ? EOK : EIO;
}
/*----------------------------------------------------------------------------*/
int pci_disable_legacy(ddf_dev_t *device)
{
	assert(device);
	int parent_phone =
	    devman_parent_device_connect(device->handle, IPC_FLAG_BLOCKING);
	if (parent_phone < 0) {
		return parent_phone;
	}

	/* See UHCI design guide for these values,
	 * write all WC bits in USB legacy register */
	sysarg_t address = 0xc0;
	sysarg_t value = 0x8f00;

	int rc = async_req_3_0(parent_phone, DEV_IFACE_ID(PCI_DEV_IFACE),
	    IPC_M_CONFIG_SPACE_WRITE_16, address, value);
	async_hangup(parent_phone);

	return rc;
}
/*----------------------------------------------------------------------------*/
/**
 * @}
 */

/**
 * @}
 */
