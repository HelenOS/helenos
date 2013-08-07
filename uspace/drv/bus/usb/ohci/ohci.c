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
#define CHECK_RET_RETURN(ret, message...) \
if (ret != EOK) { \
	usb_log_error(message); \
	return ret; \
}

	uintptr_t reg_base = 0;
	size_t reg_size = 0;
	int irq = 0;

	int ret = get_my_registers(device, &reg_base, &reg_size, &irq);
	CHECK_RET_RETURN(ret, "Failed to get register memory addresses for %"
	    PRIun ": %s.\n", ddf_dev_get_handle(device), str_error(ret));

	usb_log_debug("Memory mapped regs at %p (size %zu), IRQ %d.\n",
	    (void *) reg_base, reg_size, irq);

	const size_t ranges_count = hc_irq_pio_range_count();
	const size_t cmds_count = hc_irq_cmd_count();
	irq_pio_range_t irq_ranges[ranges_count];
	irq_cmd_t irq_cmds[cmds_count];
	irq_code_t irq_code = {
		.rangecount = ranges_count,
		.ranges = irq_ranges,
		.cmdcount = cmds_count,
		.cmds = irq_cmds
	};

	ret = hc_get_irq_code(irq_ranges, sizeof(irq_ranges), irq_cmds,
	    sizeof(irq_cmds), reg_base, reg_size);
	CHECK_RET_RETURN(ret, "Failed to gen IRQ code: %s.\n", str_error(ret));

	/* Register handler to avoid interrupt lockup */
	ret = register_interrupt_handler(device, irq, irq_handler, &irq_code);
	CHECK_RET_RETURN(ret,
	    "Failed to register irq handler: %s.\n", str_error(ret));

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

	/* Initialize generic HCD driver */
	ret = hcd_ddf_setup_device(device, NULL, USB_SPEED_FULL,
	    BANDWIDTH_AVAILABLE_USB11, bandwidth_count_usb11);
	if (ret != EOK) {
		unregister_interrupt_handler(device, irq);
		return ret;
	}

// TODO: Undo hcd_setup_device
#define CHECK_RET_CLEAN_RETURN(ret, message...) \
if (ret != EOK) { \
	unregister_interrupt_handler(device, irq); \
	CHECK_RET_RETURN(ret, message); \
} else (void)0

	hc_t *hc_impl = malloc(sizeof(hc_t));
	ret = hc_impl ? EOK : ENOMEM;
	CHECK_RET_CLEAN_RETURN(ret, "Failed to allocate driver structure.\n");

	/* Initialize OHCI HC */
	ret = hc_init(hc_impl, reg_base, reg_size, interrupts);
	CHECK_RET_CLEAN_RETURN(ret, "Failed to init hc: %s.\n", str_error(ret));

	/* Connect OHCI to generic HCD */
	hcd_set_implementation(dev_to_hcd(device), hc_impl,
	    hc_schedule, ohci_endpoint_init, ohci_endpoint_fini);

	/* HC should be running OK. We can add root hub */
	ret = hcd_ddf_setup_root_hub(device);
	CHECK_RET_CLEAN_RETURN(ret,
	    "Failed to register OHCI root hub: %s.\n", str_error(ret));

	return ret;
}
/**
 * @}
 */
