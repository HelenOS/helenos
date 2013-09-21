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

#include <errno.h>
#include <str_error.h>
#include <ddf/interrupt.h>
#include <usb/debug.h>
#include <usb/host/hcd.h>
#include <usb/host/ddf_helpers.h>

#include "uhci.h"

#include "res.h"
#include "hc.h"


/** IRQ handling callback, forward status from call to diver structure.
 *
 * @param[in] dev DDF instance of the device to use.
 * @param[in] iid (Unused).
 * @param[in] call Pointer to the call from kernel.
 */
static void irq_handler(ddf_dev_t *dev, ipc_callid_t iid, ipc_call_t *call)
{
	assert(dev);
	hcd_t *hcd = dev_to_hcd(dev);
	if (!hcd || !hcd->driver.data) {
		usb_log_error("Interrupt on not yet initialized device.\n");
		return;
	}
	const uint16_t status = IPC_GET_ARG1(*call);
	hc_interrupt(hcd->driver.data, status);
}

/** Initialize hc and rh DDF structures and their respective drivers.
 *
 * @param[in] device DDF instance of the device to use.
 *
 * This function does all the preparatory work for hc and rh drivers:
 *  - gets device's hw resources
 *  - disables UHCI legacy support (PCI config space)
 *  - attempts to enable interrupts
 *  - registers interrupt handler
 */
int device_setup_uhci(ddf_dev_t *device)
{
	assert(device);

	hw_res_list_parsed_t hw_res;
	int ret = hcd_ddf_get_registers(device, &hw_res);
	if (ret != EOK ||
	    hw_res.irqs.count != 1 || hw_res.io_ranges.count != 1) {
		usb_log_error("Failed to get register memory addresses "
		    "for %" PRIun ": %s.\n", ddf_dev_get_handle(device),
		    str_error(ret));
		return ret;
	}
	addr_range_t regs = hw_res.io_ranges.ranges[0];
	const int irq = hw_res.irqs.irqs[0];
	hw_res_list_parsed_clean(&hw_res);

	usb_log_debug("I/O regs at %p (size %zu), IRQ %d.\n",
	    RNGABSPTR(regs), RNGSZ(regs), irq);

	ret = hcd_ddf_setup_hc(device, USB_SPEED_FULL,
	    BANDWIDTH_AVAILABLE_USB11, bandwidth_count_usb11);
	if (ret != EOK) {
		usb_log_error("Failed to setup generic HCD.\n");
		return ret;
	}

	hc_t *hc = malloc(sizeof(hc_t));
	if (!hc) {
		usb_log_error("Failed to allocate UHCI HC structure.\n");
		ret = ENOMEM;
		goto ddf_hc_clean;
	}

	bool interrupts = false;
	ret = hcd_ddf_setup_interrupts(device, &regs, irq, irq_handler,
	    hc_gen_irq_code);
	if (ret != EOK) {
		usb_log_warning("Failed to enable interrupts: %s."
		    " Falling back to polling.\n", str_error(ret));
		unregister_interrupt_handler(device, irq);
	} else {
		usb_log_debug("Hw interrupts enabled.\n");
		interrupts = true;
	}

	ret = disable_legacy(device);
	if (ret != EOK) {
		usb_log_error("Failed to disable legacy USB: %s.\n",
		    str_error(ret));
		goto irq_unregister;
	}

	ret = hc_init(hc, &regs, interrupts);
	if (ret != EOK) {
		usb_log_error("Failed to init uhci_hcd: %s.\n", str_error(ret));
		goto irq_unregister;
		// TODO This is unfortunate, we have neither legacy nor real USB
	}

	hcd_set_implementation(dev_to_hcd(device), hc, hc_schedule, NULL, NULL);

	/*
	 * Creating root hub registers a new USB device so HC
	 * needs to be ready at this time.
	 */
	ret = hcd_ddf_setup_root_hub(device);
	if (ret != EOK) {
		usb_log_error("Failed to setup UHCI root hub: %s.\n",
		    str_error(ret));
		hc_fini(hc);
irq_unregister:
		unregister_interrupt_handler(device, irq);
		free(hc);
ddf_hc_clean:
		hcd_ddf_clean_hc(device);
	}
	return ret;
}
/**
 * @}
 */
