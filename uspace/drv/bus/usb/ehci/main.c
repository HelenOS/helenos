/*
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * Main routines of EHCI driver.
 */
#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <device/hw_res.h>
#include <errno.h>
#include <str_error.h>

#include <usb_iface.h>
#include <usb/debug.h>
#include <usb/host/ddf_helpers.h>

#include "res.h"

#define NAME "ehci"

static int ehci_dev_add(ddf_dev_t *device);

static driver_ops_t ehci_driver_ops = {
	.dev_add = ehci_dev_add,
};

static driver_t ehci_driver = {
	.name = NAME,
	.driver_ops = &ehci_driver_ops
};


/** Initializes a new ddf driver instance of EHCI hcd.
 *
 * @param[in] device DDF instance of the device to initialize.
 * @return Error code.
 */
static int ehci_dev_add(ddf_dev_t *device)
{
	assert(device);
	hw_res_list_parsed_t hw_res;
	int ret = hcd_ddf_get_registers(device, &hw_res);
	if (ret != EOK ||
	    hw_res.irqs.count != 1 || hw_res.mem_ranges.count != 1) {
		usb_log_error("Failed to get register memory addresses "
		    "for %" PRIun ": %s.\n", ddf_dev_get_handle(device),
		    str_error(ret));
		return ret;
	}
	addr_range_t regs = hw_res.mem_ranges.ranges[0];
	const int irq = hw_res.irqs.irqs[0];
	hw_res_list_parsed_clean(&hw_res);

	usb_log_info("Memory mapped regs at %p (size %zu), IRQ %d.\n",
	    RNGABSPTR(regs), RNGSZ(regs), irq);

	ret = disable_legacy(device, &regs);
	if (ret != EOK) {
		usb_log_error("Failed to disable legacy USB: %s.\n",
		    str_error(ret));
		return ret;
	}

	/* High Speed, no bandwidth */
	ret = hcd_ddf_setup_hc(device, USB_SPEED_HIGH, 0, NULL);
	if (ret != EOK) {
		usb_log_error("Failed to init generci hcd driver: %s\n",
		    str_error(ret));
		return ret;
	}

	usb_log_info("Controlling new EHCI device `%s' (handle %" PRIun ").\n",
	    ddf_dev_get_name(device), ddf_dev_get_handle(device));

	return EOK;
}

/** Initializes global driver structures (NONE).
 *
 * @param[in] argc Nmber of arguments in argv vector (ignored).
 * @param[in] argv Cmdline argument vector (ignored).
 * @return Error code.
 *
 * Driver debug level is set here.
 */
int main(int argc, char *argv[])
{
	log_init(NAME);
	return ddf_driver_main(&ehci_driver);
}
/**
 * @}
 */
