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
#include <str_error.h>
#include <assert.h>
#include <devman.h>
#include <ddi.h>
#include <usb/debug.h>
#include <device/hw_res_parsed.h>
#include <pci_dev_iface.h>

#include "res.h"

#define HCC_PARAMS_OFFSET 0x8
#define HCC_PARAMS_EECP_MASK 0xff
#define HCC_PARAMS_EECP_OFFSET 8

#define CMD_OFFSET 0x0
#define STS_OFFSET 0x4
#define INT_OFFSET 0x8
#define CFG_OFFSET 0x40

#define USBCMD_RUN 1
#define USBSTS_HALTED (1 << 12)

#define USBLEGSUP_OFFSET 0
#define USBLEGSUP_BIOS_CONTROL (1 << 16)
#define USBLEGSUP_OS_CONTROL (1 << 24)
#define USBLEGCTLSTS_OFFSET 4

#define DEFAULT_WAIT 1000
#define WAIT_STEP 10


/** Get address of registers and IRQ for given device.
 *
 * @param[in] dev Device asking for the addresses.
 * @param[out] mem_regs_p Pointer to the register range.
 * @param[out] irq_no IRQ assigned to the device.
 * @return Error code.
 */
int get_my_registers(ddf_dev_t *dev,
    addr_range_t *mem_regs_p, int *irq_no)
{
	assert(dev);
	
	async_sess_t *parent_sess = devman_parent_device_connect(
	    EXCHANGE_SERIALIZE, ddf_dev_get_handle(dev), IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;
	
	hw_res_list_parsed_t hw_res;
	hw_res_list_parsed_init(&hw_res);
	const int ret = hw_res_get_list_parsed(parent_sess, &hw_res, 0);
	async_hangup(parent_sess);
	if (ret != EOK) {
		return ret;
	}

	if (hw_res.irqs.count != 1 || hw_res.mem_ranges.count != 1) {
		hw_res_list_parsed_clean(&hw_res);
		return ENOENT;
	}

	if (mem_regs_p)
		*mem_regs_p = hw_res.mem_ranges.ranges[0];
	if (irq_no)
		*irq_no = hw_res.irqs.irqs[0];

	hw_res_list_parsed_clean(&hw_res);
	return EOK;
}

/** Calls the PCI driver with a request to enable interrupts
 *
 * @param[in] device Device asking for interrupts
 * @return Error code.
 */
int enable_interrupts(ddf_dev_t *device)
{
	async_sess_t *parent_sess = devman_parent_device_connect(
	    EXCHANGE_SERIALIZE, ddf_dev_get_handle(device), IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;
	
	const bool enabled = hw_res_enable_interrupt(parent_sess);
	async_hangup(parent_sess);
	
	return enabled ? EOK : EIO;
}

/** Implements BIOS hands-off routine as described in EHCI spec
 *
 * @param device EHCI device
 * @param eecp Value of EHCI Extended Capabilities pointer.
 * @return Error code.
 */
static int disable_extended_caps(ddf_dev_t *device, unsigned eecp)
{
	/* nothing to do */
	if (eecp == 0)
		return EOK;

	async_sess_t *parent_sess = devman_parent_device_connect(
	    EXCHANGE_SERIALIZE, ddf_dev_get_handle(device), IPC_FLAG_BLOCKING);
	if (!parent_sess)
		return ENOMEM;

	/* Read the first EEC. i.e. Legacy Support register */
	uint32_t usblegsup;
	int rc = pci_config_space_read_32(parent_sess,
	    eecp + USBLEGSUP_OFFSET, &usblegsup);
	if (rc != EOK) {
		usb_log_error("Failed to read USBLEGSUP: %s.\n",
		    str_error(rc));
		goto error;
	}

	usb_log_debug("USBLEGSUP: %" PRIx32 ".\n", usblegsup);

	/* Request control from firmware/BIOS by writing 1 to highest
	 * byte. (OS Control semaphore)*/
	usb_log_debug("Requesting OS control.\n");
	rc = pci_config_space_write_8(parent_sess,
	    eecp + USBLEGSUP_OFFSET + 3, 1);
	if (rc != EOK) {
		usb_log_error("Failed to request OS EHCI control: %s.\n",
		    str_error(rc));
		goto error;
	}

	size_t wait = 0;
	/* Wait for BIOS to release control. */
	rc = pci_config_space_read_32(
	    parent_sess, eecp + USBLEGSUP_OFFSET, &usblegsup);
	if (rc != EOK) {
		usb_log_error("Failed reading PCI config space: %s.\n",
		    str_error(rc));
		goto error;
	}

	while ((wait < DEFAULT_WAIT) && (usblegsup & USBLEGSUP_BIOS_CONTROL)) {
		async_usleep(WAIT_STEP);
		rc = pci_config_space_read_32(parent_sess,
		    eecp + USBLEGSUP_OFFSET, &usblegsup);
		if (rc != EOK) {
			usb_log_error("Failed reading PCI config space: %s.\n",
			    str_error(rc));
			goto error;
		}
		wait += WAIT_STEP;
	}

	if ((usblegsup & USBLEGSUP_BIOS_CONTROL) == 0) {
		usb_log_info("BIOS released control after %zu usec.\n", wait);
		async_hangup(parent_sess);
		return EOK;
	}

	/* BIOS failed to hand over control, this should not happen. */
	usb_log_warning( "BIOS failed to release control after "
	    "%zu usecs, force it.\n", wait);
	rc = pci_config_space_write_32(parent_sess,
	    eecp + USBLEGSUP_OFFSET, USBLEGSUP_OS_CONTROL);
	if (rc != EOK) {
		usb_log_error("Failed to force OS control: "
		    "%s.\n", str_error(rc));
		goto error;
	}

	/*
	 * Check capability type here, value of 01h identifies the capability
	 * as Legacy Support. This extended capability requires one additional
	 * 32-bit register for control/status information and this register is
	 * located at offset EECP+04h
	 */
	if ((usblegsup & 0xff) == 1) {
		/* Read the second EEC Legacy Support and Control register */
		uint32_t usblegctlsts;
		rc = pci_config_space_read_32(parent_sess,
		    eecp + USBLEGCTLSTS_OFFSET, &usblegctlsts);
		if (rc != EOK) {
			usb_log_error("Failed to get USBLEGCTLSTS: %s.\n",
			    str_error(rc));
			goto error;
		}

		usb_log_debug("USBLEGCTLSTS: %" PRIx32 ".\n", usblegctlsts);
		/*
		 * Zero SMI enables in legacy control register.
		 * It should prevent pre-OS code from
		 * interfering. NOTE: Three upper bits are WC
		 */
		rc = pci_config_space_write_32(parent_sess,
		    eecp + USBLEGCTLSTS_OFFSET, 0xe0000000);
		if (rc != EOK) {
			usb_log_error("Failed(%d) zero USBLEGCTLSTS.\n", rc);
			goto error;
		}

		udelay(10);
		rc = pci_config_space_read_32(parent_sess,
		    eecp + USBLEGCTLSTS_OFFSET, &usblegctlsts);
		if (rc != EOK) {
			usb_log_error("Failed to get USBLEGCTLSTS 2: %s.\n",
			    str_error(rc));
			goto error;
		}

		usb_log_debug("Zeroed USBLEGCTLSTS: %" PRIx32 ".\n",
		    usblegctlsts);
	}

	/* Read again Legacy Support register */
	rc = pci_config_space_read_32(parent_sess,
	    eecp + USBLEGSUP_OFFSET, &usblegsup);
	if (rc != EOK) {
		usb_log_error("Failed to read USBLEGSUP: %s.\n",
		    str_error(rc));
		goto error;
	}

	usb_log_debug("USBLEGSUP: %" PRIx32 ".\n", usblegsup);
	async_hangup(parent_sess);
	return EOK;
error:
	async_hangup(parent_sess);
	return rc;
}

int disable_legacy(ddf_dev_t *device, addr_range_t *reg_range)
{
	assert(device);
	usb_log_debug("Disabling EHCI legacy support.\n");

	/* Map EHCI registers */
	void *regs = NULL;
	int rc = pio_enable_range(reg_range, &regs);
	if (rc != EOK) {
		usb_log_error("Failed to map registers %p: %s.\n",
		    RNGABSPTR(*reg_range), str_error(rc));
		return rc;
	}

	usb_log_debug2("Registers mapped at: %p.\n", regs);

	const uint32_t hcc_params =
	    *(uint32_t*)(regs + HCC_PARAMS_OFFSET);
	usb_log_debug("Value of hcc params register: %x.\n", hcc_params);

	/* Read value of EHCI Extended Capabilities Pointer
	 * position of EEC registers (points to PCI config space) */
	const uint32_t eecp =
	    (hcc_params >> HCC_PARAMS_EECP_OFFSET) & HCC_PARAMS_EECP_MASK;
	usb_log_debug("Value of EECP: %x.\n", eecp);

	rc = disable_extended_caps(device, eecp);
	if (rc != EOK) {
		usb_log_error("Failed to disable extended capabilities: %s.\n",
		    str_error(rc));
		return rc;
	}

	/*
	 * TURN OFF EHCI FOR NOW, DRIVER WILL REINITIALIZE IT IF NEEDED
	 */

	/* Get size of capability registers in memory space. */
	const unsigned operation_offset = *(uint8_t*)regs;
	usb_log_debug("USBCMD offset: %d.\n", operation_offset);

	/* Zero USBCMD register. */
	volatile uint32_t *usbcmd =
	    (uint32_t*)((uint8_t*)regs + operation_offset + CMD_OFFSET);
	volatile uint32_t *usbsts =
	    (uint32_t*)((uint8_t*)regs + operation_offset + STS_OFFSET);
	volatile uint32_t *usbconf =
	    (uint32_t*)((uint8_t*)regs + operation_offset + CFG_OFFSET);
	volatile uint32_t *usbint =
	    (uint32_t*)((uint8_t*)regs + operation_offset + INT_OFFSET);
	usb_log_debug("USBCMD value: %x.\n", *usbcmd);
	if (*usbcmd & USBCMD_RUN) {
		*usbsts = 0x3f; /* ack all interrupts */
		*usbint = 0; /* disable all interrupts */
		*usbconf = 0; /* release control of RH ports */

		*usbcmd = 0;
		/* Wait until hc is halted */
		while ((*usbsts & USBSTS_HALTED) == 0);
		usb_log_info("EHCI turned off.\n");
	} else {
		usb_log_info("EHCI was not running.\n");
	}
	usb_log_debug("Registers: \n"
	    "\t USBCMD(%p): %x(0x00080000 = at least 1ms between interrupts)\n"
	    "\t USBSTS(%p): %x(0x00001000 = HC halted)\n"
	    "\t USBINT(%p): %x(0x0 = no interrupts).\n"
	    "\t CONFIG(%p): %x(0x0 = ports controlled by companion hc).\n",
	    usbcmd, *usbcmd, usbsts, *usbsts, usbint, *usbint, usbconf,*usbconf);

	return rc;
}

/**
 * @}
 */
