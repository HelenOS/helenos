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
#include <devman.h>
#include <device/hw_res_parsed.h>

#include <usb/debug.h>

#include "res.h"

/** Get address of registers and IRQ for given device.
 *
 * @param[in] dev Device asking for the addresses.
 * @param[out] mem_reg_address Base address of the memory range.
 * @param[out] mem_reg_size Size of the memory range.
 * @param[out] irq_no IRQ assigned to the device.
 * @return Error code.
 */
int get_my_registers(const ddf_dev_t *dev,
    uintptr_t *mem_reg_address, size_t *mem_reg_size, int *irq_no)
{
	assert(dev);

	async_sess_t *parent_sess =
	    devman_parent_device_connect(EXCHANGE_SERIALIZE, dev->handle,
	    IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;

	hw_res_list_parsed_t hw_res;
	hw_res_list_parsed_init(&hw_res);
	const int ret =  hw_res_get_list_parsed(parent_sess, &hw_res, 0);
	async_hangup(parent_sess);
	if (ret != EOK) {
		return ret;
	}

	/* We want one irq and one mem range. */
	if (hw_res.irqs.count != 1 || hw_res.mem_ranges.count != 1) {
		hw_res_list_parsed_clean(&hw_res);
		return EINVAL;
	}

	if (mem_reg_address)
		*mem_reg_address = hw_res.mem_ranges.ranges[0].address;
	if (mem_reg_size)
		*mem_reg_size = hw_res.mem_ranges.ranges[0].size;
	if (irq_no)
		*irq_no = hw_res.irqs.irqs[0];

	hw_res_list_parsed_clean(&hw_res);
	return EOK;
}

/** Call the PCI driver with a request to enable interrupts
 *
 * @param[in] device Device asking for interrupts
 * @return Error code.
 */
int enable_interrupts(const ddf_dev_t *device)
{
	async_sess_t *parent_sess =
	    devman_parent_device_connect(EXCHANGE_SERIALIZE, device->handle,
	    IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;
	
	const bool enabled = hw_res_enable_interrupt(parent_sess);
	async_hangup(parent_sess);
	
	return enabled ? EOK : EIO;
}

/**
 * @}
 */
