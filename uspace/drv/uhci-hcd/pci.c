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
#include "pci.h"
#include <errno.h>
#include <assert.h>
#include <devman.h>
#include <device/hw_res.h>

/** Get address of registers and IRQ for given device.
 *
 * @param[in] dev Device asking for the addresses.
 * @param[out] io_reg_address Base address of the I/O range.
 * @param[out] io_reg_size Size of the I/O range.
 * @param[out] irq_no IRQ assigned to the device.
 * @return Error code.
 */
int pci_get_my_registers(device_t *dev,
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
		switch (res->type) {
			case INTERRUPT:
				irq = res->res.interrupt.irq;
				irq_found = true;
				break;
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

	if (!irq_found) {
		rc = ENOENT;
		goto leave;
	}

	if (io_reg_address != NULL) {
		*io_reg_address = io_address;
	}
	if (io_reg_size != NULL) {
		*io_reg_size = io_size;
	}
	if (irq_no != NULL) {
		*irq_no = irq;
	}

	rc = EOK;
leave:
	ipc_hangup(parent_phone);

	return rc;
}

/**
 * @}
 */

