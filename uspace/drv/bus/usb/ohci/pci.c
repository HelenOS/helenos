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

/**
 * @addtogroup drvusbohci
 * @{
 */
/**
 * @file
 * PCI related functions needed by the OHCI driver.
 */

#include <errno.h>
#include <assert.h>
#include <as.h>
#include <devman.h>
#include <ddi.h>
#include <libarch/ddi.h>
#include <device/hw_res.h>

#include <usb/debug.h>
#include <pci_dev_iface.h>

#include "pci.h"

/** Get address of registers and IRQ for given device.
 *
 * @param[in] dev Device asking for the addresses.
 * @param[out] mem_reg_address Base address of the memory range.
 * @param[out] mem_reg_size Size of the memory range.
 * @param[out] irq_no IRQ assigned to the device.
 * @return Error code.
 */
int pci_get_my_registers(ddf_dev_t *dev,
    uintptr_t *mem_reg_address, size_t *mem_reg_size, int *irq_no)
{
	assert(dev);
	assert(mem_reg_address);
	assert(mem_reg_size);
	assert(irq_no);

	async_sess_t *parent_sess =
	    devman_parent_device_connect(EXCHANGE_SERIALIZE, dev->handle,
	    IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;

	hw_resource_list_t hw_resources;
	int rc = hw_res_get_resource_list(parent_sess, &hw_resources);
	async_hangup(parent_sess);
	if (rc != EOK) {
		return rc;
	}

	uintptr_t mem_address = 0;
	size_t mem_size = 0;
	bool mem_found = false;

	int irq = 0;
	bool irq_found = false;

	for (size_t i = 0; i < hw_resources.count; i++) {
		hw_resource_t *res = &hw_resources.resources[i];
		switch (res->type) {
		case INTERRUPT:
			irq = res->res.interrupt.irq;
			irq_found = true;
			usb_log_debug2("Found interrupt: %d.\n", irq);
			break;
		case MEM_RANGE:
			if (res->res.mem_range.address != 0
			    && res->res.mem_range.size != 0 ) {
				mem_address = res->res.mem_range.address;
				mem_size = res->res.mem_range.size;
				usb_log_debug2("Found mem: %p %zu.\n",
				    (void *) mem_address, mem_size);
				mem_found = true;
			}
		default:
			break;
		}
	}
	free(hw_resources.resources);

	if (mem_found && irq_found) {
		*mem_reg_address = mem_address;
		*mem_reg_size = mem_size;
		*irq_no = irq;
		return EOK;
	}
	return ENOENT;
}

/** Call the PCI driver with a request to enable interrupts
 *
 * @param[in] device Device asking for interrupts
 * @return Error code.
 */
int pci_enable_interrupts(ddf_dev_t *device)
{
	async_sess_t *parent_sess =
	    devman_parent_device_connect(EXCHANGE_SERIALIZE, device->handle,
	    IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;
	
	bool enabled = hw_res_enable_interrupt(parent_sess);
	async_hangup(parent_sess);
	
	return enabled ? EOK : EIO;
}

/**
 * @}
 */
