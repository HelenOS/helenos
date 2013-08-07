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
	if (!hcd || !hcd->private_data) {
		usb_log_error("Interrupt on not yet initialized device.\n");
		return;
	}
	const uint16_t status = IPC_GET_ARG1(*call);
	hc_interrupt(hcd->private_data, status);
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
	if (!device)
		return EBADMEM;

#define CHECK_RET_RETURN(ret, message...) \
if (ret != EOK) { \
	usb_log_error(message); \
	return ret; \
} else (void)0

	uintptr_t reg_base = 0;
	size_t reg_size = 0;
	int irq = 0;

	int ret = get_my_registers(device, &reg_base, &reg_size, &irq);
	CHECK_RET_RETURN(ret, "Failed to get I/O region for %" PRIun ": %s.\n",
	    ddf_dev_get_handle(device), str_error(ret));
	usb_log_debug("I/O regs at 0x%p (size %zu), IRQ %d.\n",
	    (void *) reg_base, reg_size, irq);

	const size_t ranges_count = hc_irq_pio_range_count();
	const size_t cmds_count = hc_irq_cmd_count();
	irq_pio_range_t irq_ranges[ranges_count];
	irq_cmd_t irq_cmds[cmds_count];
	ret = hc_get_irq_code(irq_ranges, sizeof(irq_ranges), irq_cmds,
	    sizeof(irq_cmds), reg_base, reg_size);
	CHECK_RET_RETURN(ret, "Failed to generate IRQ commands: %s.\n",
	    str_error(ret));

	irq_code_t irq_code = {
		.rangecount = ranges_count,
		.ranges = irq_ranges,
		.cmdcount = cmds_count,
		.cmds = irq_cmds
	};

        /* Register handler to avoid interrupt lockup */
        ret = register_interrupt_handler(device, irq, irq_handler, &irq_code);
        CHECK_RET_RETURN(ret, "Failed to register interrupt handler: %s.\n",
	    str_error(ret));
	
	ret = disable_legacy(device);
	CHECK_RET_RETURN(ret, "Failed to disable legacy USB: %s.\n",
	    str_error(ret));

	bool interrupts = false;
	ret = enable_interrupts(device);
	if (ret != EOK) {
		usb_log_warning("Failed to enable interrupts: %s."
		    " Falling back to polling.\n", str_error(ret));
	} else {
		usb_log_debug("Hw interrupts enabled.\n");
		interrupts = true;
	}

	ret = hcd_ddf_setup_device(device, NULL, USB_SPEED_FULL,
	    BANDWIDTH_AVAILABLE_USB11, bandwidth_count_usb11);
	CHECK_RET_RETURN(ret, "Failed to setup UHCI HCD.\n");
	
	hc_t *hc = malloc(sizeof(hc_t));
	ret = hc ? EOK : ENOMEM;
	CHECK_RET_RETURN(ret, "Failed to allocate UHCI HC structure.\n");

	ret = hc_init(hc, (void*)reg_base, reg_size, interrupts);
	CHECK_RET_RETURN(ret,
	    "Failed to init uhci_hcd: %s.\n", str_error(ret));

	hcd_set_implementation(dev_to_hcd(device), hc, hc_schedule, NULL, NULL);

	/*
	 * Creating root hub registers a new USB device so HC
	 * needs to be ready at this time.
	 */
	ret = hcd_ddf_setup_root_hub(device);
	if (ret != EOK) {
		// TODO: Undo hcd_setup_device
		hc_fini(hc);
		CHECK_RET_RETURN(ret, "Failed to setup UHCI root hub: %s.\n",
		    str_error(ret));
		return ret;
	}

	return EOK;
}
/**
 * @}
 */
