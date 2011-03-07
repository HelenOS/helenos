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
 * @addtogroup drvusbehci
 * @{
 */
/**
 * @file
 * PCI related functions needed by the EHCI driver.
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

#define PAGE_SIZE_MASK 0xfffff000
#define HCC_PARAMS_OFFSET 0x8
#define HCC_PARAMS_EECP_MASK 0xff
#define HCC_PARAMS_EECP_OFFSET 8

#define USBCMD_RUN 1

#define USBLEGSUP_OFFSET 0
#define USBLEGSUP_BIOS_CONTROL (1 << 16)
#define USBLEGSUP_OS_CONTROL (1 << 24)
#define USBLEGCTLSTS_OFFSET 4

#define DEFAULT_WAIT 10000
#define WAIT_STEP 10


/** Get address of registers and IRQ for given device.
 *
 * @param[in] dev Device asking for the addresses.
 * @param[out] io_reg_address Base address of the I/O range.
 * @param[out] io_reg_size Size of the I/O range.
 * @param[out] irq_no IRQ assigned to the device.
 * @return Error code.
 */
int pci_get_my_registers(ddf_dev_t *dev,
    uintptr_t *mem_reg_address, size_t *mem_reg_size, int *irq_no)
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

	uintptr_t mem_address = 0;
	size_t mem_size = 0;
	bool mem_found = false;

	int irq = 0;
	bool irq_found = false;

	size_t i;
	for (i = 0; i < hw_resources.count; i++) {
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
					usb_log_debug2("Found mem: %llx %zu.\n",
							res->res.mem_range.address, res->res.mem_range.size);
					mem_found = true;
				}
				break;
			default:
				break;
		}
	}

	if (!mem_found) {
		rc = ENOENT;
		goto leave;
	}

	if (!irq_found) {
		rc = ENOENT;
		goto leave;
	}

	*mem_reg_address = mem_address;
	*mem_reg_size = mem_size;
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
	int parent_phone = devman_parent_device_connect(device->handle,
		IPC_FLAG_BLOCKING);
	if (parent_phone < 0) {
		return parent_phone;
	}

	/* read register space BAR */
	sysarg_t address = 0x10;
	sysarg_t value;

  int ret = async_req_2_1(parent_phone, DEV_IFACE_ID(PCI_DEV_IFACE),
	    IPC_M_CONFIG_SPACE_READ_32, address, &value);
	usb_log_info("Register space BAR at %p:%x.\n", address, value);

	/* clear lower byte, it's not part of the address */
	uintptr_t registers = (value & 0xffffff00);
	usb_log_info("Mem register address:%p.\n", registers);

	/* if nothing setup the hc, the we don't need to to turn it off */
	if (registers == 0)
		return ENOTSUP;

	/* EHCI registers need 20 bytes*/
	void *regs = as_get_mappable_page(4096);
	ret = physmem_map((void*)(registers & PAGE_SIZE_MASK), regs, 1,
	    AS_AREA_READ | AS_AREA_WRITE);
	if (ret != EOK) {
		usb_log_error("Failed(%d) to map registers %p:%p.\n",
		    ret, regs, registers);
	}
	/* calculate value of BASE */
	registers = (registers & 0xf00) | (uintptr_t)regs;

	uint32_t hcc_params = *(uint32_t*)(registers + HCC_PARAMS_OFFSET);

	usb_log_debug("Value of hcc params register: %x.\n", hcc_params);
	uint32_t eecp = (hcc_params >> HCC_PARAMS_EECP_OFFSET) & HCC_PARAMS_EECP_MASK;
	usb_log_debug("Value of EECP: %x.\n", eecp);

	ret = async_req_2_1(parent_phone, DEV_IFACE_ID(PCI_DEV_IFACE),
	    IPC_M_CONFIG_SPACE_READ_32, eecp + USBLEGCTLSTS_OFFSET, &value);
	if (ret != EOK) {
		usb_log_error("Failed(%d) to read USBLEGCTLSTS.\n", ret);
		return ret;
	}
	usb_log_debug2("USBLEGCTLSTS: %x.\n", value);

	ret = async_req_2_1(parent_phone, DEV_IFACE_ID(PCI_DEV_IFACE),
	    IPC_M_CONFIG_SPACE_READ_32, eecp + USBLEGSUP_OFFSET, &value);
	if (ret != EOK) {
		usb_log_error("Failed(%d) to read USBLEGSUP.\n", ret);
		return ret;
	}
	usb_log_debug2("USBLEGSUP: %x.\n", value);

	/* request control from firmware/BIOS, by writing 1 to highest byte */
	ret = async_req_3_0(parent_phone, DEV_IFACE_ID(PCI_DEV_IFACE),
	   IPC_M_CONFIG_SPACE_WRITE_8, eecp + USBLEGSUP_OFFSET + 3, 1);
	if (ret != EOK) {
		usb_log_error("Failed(%d) request OS EHCI control.\n", ret);
		return ret;
	}

	size_t wait = 0; /* might be anything */
	/* wait for BIOS to release control */
	while ((wait < DEFAULT_WAIT) && (value & USBLEGSUP_BIOS_CONTROL)) {
		async_usleep(WAIT_STEP);
		ret = async_req_2_1(parent_phone, DEV_IFACE_ID(PCI_DEV_IFACE),
				IPC_M_CONFIG_SPACE_READ_32, eecp + USBLEGSUP_OFFSET, &value);
		wait += WAIT_STEP;
	}

	if ((value & USBLEGSUP_BIOS_CONTROL) != 0) {
		usb_log_warning(
		    "BIOS failed to release control after %d usecs, force it.\n",
		    wait);
		ret = async_req_3_0(parent_phone, DEV_IFACE_ID(PCI_DEV_IFACE),
			  IPC_M_CONFIG_SPACE_WRITE_32, eecp + USBLEGSUP_OFFSET,
		    USBLEGSUP_OS_CONTROL);
		if (ret != EOK) {
			usb_log_error("Failed(%d) to force OS EHCI control.\n", ret);
			return ret;
		}
	} else {
		usb_log_info("BIOS released control after %d usec.\n",
		    wait);
	}


	/* zero SMI enables in legacy control register */
	ret = async_req_3_0(parent_phone, DEV_IFACE_ID(PCI_DEV_IFACE),
	   IPC_M_CONFIG_SPACE_WRITE_32, eecp + USBLEGCTLSTS_OFFSET, 0);
	if (ret != EOK) {
		usb_log_error("Failed(%d) zero USBLEGCTLSTS.\n", ret);
		return ret;
	}
	usb_log_debug("Zeroed USBLEGCTLSTS register.\n");

	ret = async_req_2_1(parent_phone, DEV_IFACE_ID(PCI_DEV_IFACE),
	    IPC_M_CONFIG_SPACE_READ_32, eecp + USBLEGCTLSTS_OFFSET, &value);
	if (ret != EOK) {
		usb_log_error("Failed(%d) to read USBLEGCTLSTS.\n", ret);
		return ret;
	}
	usb_log_debug2("USBLEGCTLSTS: %x.\n", value);

	ret = async_req_2_1(parent_phone, DEV_IFACE_ID(PCI_DEV_IFACE),
	    IPC_M_CONFIG_SPACE_READ_32, eecp + USBLEGSUP_OFFSET, &value);
	if (ret != EOK) {
		usb_log_error("Failed(%d) to read USBLEGSUP.\n", ret);
		return ret;
	}
	usb_log_debug2("USBLEGSUP: %x.\n", value);

/*
 * TURN OFF EHCI FOR NOW, REMOVE IF THE DRIVER IS IMPLEMENTED
 */

	/* size of capability registers in memory space */
	uint8_t operation_offset = *(uint8_t*)registers;
	usb_log_debug("USBCMD offset: %d.\n", operation_offset);
	/* zero USBCMD register */
	volatile uint32_t *usbcmd =
	 (uint32_t*)((uint8_t*)registers + operation_offset);
	usb_log_debug("USBCMD value: %x.\n", *usbcmd);
	if (*usbcmd & USBCMD_RUN) {
		*usbcmd = 0;
		usb_log_info("EHCI turned off.\n");
	} else {
		usb_log_info("EHCI was not running.\n");
	}


	async_hangup(parent_phone);

  return ret;
}
/*----------------------------------------------------------------------------*/
/**
 * @}
 */

/**
 * @}
 */
