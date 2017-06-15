/*
 * Copyright (c) 2017 Ondrej Hlavaty <aearsis@eideo.cz>
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

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * Main routines of XHCI driver.
 */

#include <ddf/driver.h>
#include <errno.h>
#include <io/log.h>
#include <io/logctl.h>
#include <usb/debug.h>
#include <usb/host/ddf_helpers.h>

#include "hc.h"

#define NAME "xhci"

static int xhci_driver_init(hcd_t *, const hw_res_list_parsed_t *, bool);
static void xhci_driver_fini(hcd_t *);

static const ddf_hc_driver_t xhci_ddf_hc_driver = {
	.hc_speed = USB_SPEED_SUPER,
	.irq_code_gen = xhci_hc_gen_irq_code,
	.init = xhci_driver_init,
	.fini = xhci_driver_fini,
	.name = "XHCI-PCI",
	.ops = {
		.schedule       = xhci_hc_schedule,
		.irq_hook       = xhci_hc_interrupt,
		.status_hook    = xhci_hc_status,
	}
};

static int xhci_driver_init(hcd_t *hcd, const hw_res_list_parsed_t *res, bool irq)
{
	usb_log_info("Initializing");
	return ENOTSUP;
}

static void xhci_driver_fini(hcd_t *hcd)
{
	usb_log_info("Finishing");
	assert(hcd);
}

/** Initializes a new ddf driver instance of XHCI hcd.
 *
 * @param[in] device DDF instance of the device to initialize.
 * @return Error code.
 */
static int xhci_dev_add(ddf_dev_t *device)
{
	usb_log_info("Adding device %s", ddf_dev_get_name(device));
	return hcd_ddf_add_hc(device, &xhci_ddf_hc_driver);
}


static const driver_ops_t xhci_driver_ops = {
	.dev_add = xhci_dev_add,
};

static const driver_t xhci_driver = {
	.name = NAME,
	.driver_ops = &xhci_driver_ops
};


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
	logctl_set_log_level(NAME, LVL_DEBUG2);
	return ddf_driver_main(&xhci_driver);
}

/**
 * @}
 */
