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

/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver
 */

#include <errno.h>
#include <str_error.h>
#include <ddf/interrupt.h>
#include <usb/usb.h>
#include <usb/debug.h>

#include <usb/host/ddf_helpers.h>

#include "ohci.h"
#include "res.h"
#include "hc.h"



/** IRQ handling callback, identifies device
 *
 * @param[in] dev DDF instance of the device to use.
 * @param[in] iid (Unused).
 * @param[in] call Pointer to the call that represents interrupt.
 */
static void irq_handler(ddf_dev_t *dev, ipc_callid_t iid, ipc_call_t *call)
{
	assert(dev);
	hcd_t *hcd = dev_to_hcd(dev);
	if (!hcd || !hcd->private_data) {
		usb_log_warning("Interrupt on device that is not ready.\n");
		return;
	}

	const uint16_t status = IPC_GET_ARG1(*call);
	hc_interrupt(hcd->private_data, status);
}

/** Initialize hc and rh ddf structures and their respective drivers.
 *
 * @param[in] device DDF instance of the device to use.
 * @param[in] instance OHCI structure to use.
 *
 * This function does all the preparatory work for hc and rh drivers:
 *  - gets device hw resources
 *  - disables OHCI legacy support
 *  - asks for interrupt
 *  - registers interrupt handler
 */
int device_setup_ohci(ddf_dev_t *device)
{

	addr_range_t regs;
	int irq = 0;

	int ret = get_my_registers(device, &regs, &irq);
	if (ret != EOK) {
		usb_log_error("Failed to get register memory addresses "
		    "for %" PRIun ": %s.\n", ddf_dev_get_handle(device),
		    str_error(ret));
		return ret;
	}

	usb_log_debug("Memory mapped regs at %p (size %zu), IRQ %d.\n",
	    RNGABSPTR(regs), RNGSZ(regs), irq);

	/* Initialize generic HCD driver */
	ret = hcd_ddf_setup_hc(device, USB_SPEED_FULL,
	    BANDWIDTH_AVAILABLE_USB11, bandwidth_count_usb11);
	if (ret != EOK) {
		usb_log_error("Failedd to setup generic hcd: %s.",
		    str_error(ret));
		return ret;
	}

	ret = hc_register_irq_handler(device, &regs, irq, irq_handler);
	if (ret != EOK) {
		usb_log_error("Failed to register interrupt handler: %s.\n",
		    str_error(ret));
		hcd_ddf_clean_hc(device);
		return ret;
	}

	/* Try to enable interrupts */
	bool interrupts = false;
	ret = enable_interrupts(device);
	if (ret != EOK) {
		usb_log_warning("Failed to enable interrupts: %s."
		    " Falling back to polling\n", str_error(ret));
		/* We don't need that handler */
		unregister_interrupt_handler(device, irq);
	} else {
		usb_log_debug("Hw interrupts enabled.\n");
		interrupts = true;
	}


	hc_t *hc_impl = malloc(sizeof(hc_t));
	if (!hc_impl) {
		usb_log_error("Failed to allocate driver structure.\n");
		hcd_ddf_clean_hc(device);
		unregister_interrupt_handler(device, irq);
		return ret;
	}

	/* Initialize OHCI HC */
	ret = hc_init(hc_impl, &regs, interrupts);
	if (ret != EOK) {
		usb_log_error("Failed to init hc: %s.\n", str_error(ret));
		hcd_ddf_clean_hc(device);
		unregister_interrupt_handler(device, irq);
		return ret;
	}

	/* Connect OHCI to generic HCD */
	hcd_set_implementation(dev_to_hcd(device), hc_impl,
	    hc_schedule, ohci_endpoint_init, ohci_endpoint_fini);

	/* HC should be running OK. We can add root hub */
	ret = hcd_ddf_setup_root_hub(device);
	if (ret != EOK) {
		usb_log_error("Failed to registter OHCI root hub: %s.\n",
		    str_error(ret));
		hcd_ddf_clean_hc(device);
		unregister_interrupt_handler(device, irq);
		return ret;
	}

	return ret;
}
/**
 * @}
 */
