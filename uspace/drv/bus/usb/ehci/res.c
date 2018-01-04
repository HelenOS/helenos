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
#include <ddf/driver.h>
#include <ddi.h>
#include <usb/debug.h>
#include <device/hw_res_parsed.h>
#include <pci_dev_iface.h>

#include "res.h"
#include "ehci_regs.h"

#define USBLEGSUP_OFFSET 0
#define USBLEGSUP_BIOS_CONTROL (1 << 16)
#define USBLEGSUP_OS_CONTROL (1 << 24)
#define USBLEGCTLSTS_OFFSET 4

#define DEFAULT_WAIT 1000
#define WAIT_STEP 10

/** Implements BIOS hands-off routine as described in EHCI spec
 *
 * @param device EHCI device
 * @param eecp Value of EHCI Extended Capabilities pointer.
 * @return Error code.
 */
static errno_t disable_extended_caps(async_sess_t *parent_sess, unsigned eecp)
{
	/* nothing to do */
	if (eecp == 0)
		return EOK;

	/* Read the first EEC. i.e. Legacy Support register */
	uint32_t usblegsup;
	errno_t ret = pci_config_space_read_32(parent_sess,
	    eecp + USBLEGSUP_OFFSET, &usblegsup);
	if (ret != EOK) {
		usb_log_error("Failed to read USBLEGSUP: %s.\n", str_error(ret));
		return ret;
	}
	usb_log_debug2("USBLEGSUP: %" PRIx32 ".\n", usblegsup);

	/* Request control from firmware/BIOS by writing 1 to highest
	 * byte. (OS Control semaphore)*/
	usb_log_debug("Requesting OS control.\n");
	ret = pci_config_space_write_8(parent_sess,
	    eecp + USBLEGSUP_OFFSET + 3, 1);
	if (ret != EOK) {
		usb_log_error("Failed to request OS EHCI control: %s.\n",
		    str_error(ret));
		return ret;
	}

	size_t wait = 0;
	/* Wait for BIOS to release control. */
	ret = pci_config_space_read_32(
	    parent_sess, eecp + USBLEGSUP_OFFSET, &usblegsup);
	while ((ret == EOK) && (wait < DEFAULT_WAIT)
	    && (usblegsup & USBLEGSUP_BIOS_CONTROL)) {
		async_usleep(WAIT_STEP);
		ret = pci_config_space_read_32(parent_sess,
		    eecp + USBLEGSUP_OFFSET, &usblegsup);
		wait += WAIT_STEP;
	}

	if ((usblegsup & USBLEGSUP_BIOS_CONTROL) == 0) {
		usb_log_info("BIOS released control after %zu usec.\n", wait);
		return EOK;
	}

	/* BIOS failed to hand over control, this should not happen. */
	usb_log_warning( "BIOS failed to release control after "
	    "%zu usecs, force it.\n", wait);
	ret = pci_config_space_write_32(parent_sess,
	    eecp + USBLEGSUP_OFFSET, USBLEGSUP_OS_CONTROL);
	if (ret != EOK) {
		usb_log_error("Failed to force OS control: %s.\n",
		    str_error(ret));
		return ret;
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
		ret = pci_config_space_read_32(parent_sess,
		    eecp + USBLEGCTLSTS_OFFSET, &usblegctlsts);
		if (ret != EOK) {
			usb_log_error("Failed to get USBLEGCTLSTS: %s.\n",
			    str_error(ret));
			return ret;
		}
		usb_log_debug2("USBLEGCTLSTS: %" PRIx32 ".\n", usblegctlsts);
		/*
		 * Zero SMI enables in legacy control register.
		 * It should prevent pre-OS code from
		 * interfering. NOTE: Three upper bits are WC
		 */
		ret = pci_config_space_write_32(parent_sess,
		    eecp + USBLEGCTLSTS_OFFSET, 0xe0000000);
		if (ret != EOK) {
			usb_log_error("Failed to zero USBLEGCTLSTS: %s\n",
			    str_error(ret));
			return ret;
		}

		udelay(10);
		/* read again to amke sure it's zeroed */
		ret = pci_config_space_read_32(parent_sess,
		    eecp + USBLEGCTLSTS_OFFSET, &usblegctlsts);
		if (ret != EOK) {
			usb_log_error("Failed to get USBLEGCTLSTS 2: %s.\n",
			    str_error(ret));
			return ret;
		}
		usb_log_debug2("Zeroed USBLEGCTLSTS: %" PRIx32 ".\n",
		    usblegctlsts);
	}

	/* Read again Legacy Support register */
	ret = pci_config_space_read_32(parent_sess,
	    eecp + USBLEGSUP_OFFSET, &usblegsup);
	if (ret != EOK) {
		usb_log_error("Failed to read USBLEGSUP: %s.\n",
		    str_error(ret));
		return ret;
	}
	usb_log_debug2("USBLEGSUP: %" PRIx32 ".\n", usblegsup);
	return ret;
}

errno_t disable_legacy(ddf_dev_t *device)
{
	assert(device);

	async_sess_t *parent_sess = ddf_dev_parent_sess_get(device);
	if (parent_sess == NULL)
		return ENOMEM;

	usb_log_debug("Disabling EHCI legacy support.\n");

	hw_res_list_parsed_t res;
	hw_res_list_parsed_init(&res);
	errno_t ret = hw_res_get_list_parsed(parent_sess, &res, 0);
	if (ret != EOK) {
		usb_log_error("Failed to get resource list: %s\n",
		    str_error(ret));
		goto clean;
	}

	if (res.mem_ranges.count < 1) {
		usb_log_error("Incorrect mem range count: %zu",
		    res.mem_ranges.count);
		ret = EINVAL;
		goto clean;
	}

	/* Map EHCI registers */
	void *regs = NULL;
	ret = pio_enable_range(&res.mem_ranges.ranges[0], &regs);
	if (ret != EOK) {
		usb_log_error("Failed to map registers %p: %s.\n",
		    RNGABSPTR(res.mem_ranges.ranges[0]), str_error(ret));
		goto clean;
	}

	usb_log_debug("Registers mapped at: %p.\n", regs);

	ehci_caps_regs_t *ehci_caps = regs;

	const uint32_t hcc_params = EHCI_RD(ehci_caps->hccparams);
	usb_log_debug2("Value of hcc params register: %x.\n", hcc_params);

	/* Read value of EHCI Extended Capabilities Pointer
	 * position of EEC registers (points to PCI config space) */
	const uint32_t eecp =
	    (hcc_params >> EHCI_CAPS_HCC_EECP_SHIFT) & EHCI_CAPS_HCC_EECP_MASK;
	usb_log_debug2("Value of EECP: %x.\n", eecp);

	ret = disable_extended_caps(parent_sess, eecp);
	if (ret != EOK) {
		usb_log_error("Failed to disable extended capabilities: %s.\n",
		    str_error(ret));
		    goto clean;
	}
clean:
	//TODO unmap registers
	hw_res_list_parsed_clean(&res);
	return ret;
}

/**
 * @}
 */
