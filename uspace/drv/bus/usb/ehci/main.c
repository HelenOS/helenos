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
#include <io/logctl.h>

#include <usb_iface.h>
#include <usb/debug.h>
#include <usb/host/ddf_helpers.h>

#include "res.h"
#include "hc.h"
#include "ehci_endpoint.h"

#define NAME "ehci"

static errno_t ehci_driver_init(hcd_t *, const hw_res_list_parsed_t *, bool);
static void ehci_driver_fini(hcd_t *);

static const ddf_hc_driver_t ehci_hc_driver = {
	.claim = disable_legacy,
	.hc_speed = USB_SPEED_HIGH,
	.irq_code_gen = ehci_hc_gen_irq_code,
	.init = ehci_driver_init,
	.fini = ehci_driver_fini,
	.name = "EHCI-PCI",
	.ops = {
		.schedule       = ehci_hc_schedule,
		.ep_add_hook    = ehci_endpoint_init,
		.ep_remove_hook = ehci_endpoint_fini,
		.irq_hook       = ehci_hc_interrupt,
		.status_hook    = ehci_hc_status,
	}
};


static errno_t ehci_driver_init(hcd_t *hcd, const hw_res_list_parsed_t *res,
    bool irq)
{
	assert(hcd);
	assert(hcd_get_driver_data(hcd) == NULL);

	hc_t *instance = malloc(sizeof(hc_t));
	if (!instance)
		return ENOMEM;

	const errno_t ret = hc_init(instance, res, irq);
	if (ret == EOK) {
		hcd_set_implementation(hcd, instance, &ehci_hc_driver.ops);
	} else {
		free(instance);
	}
	return ret;
}

static void ehci_driver_fini(hcd_t *hcd)
{
	assert(hcd);
	hc_t *hc = hcd_get_driver_data(hcd);
	if (hc)
		hc_fini(hc);

	free(hc);
	hcd_set_implementation(hcd, NULL, NULL);
}

/** Initializes a new ddf driver instance of EHCI hcd.
 *
 * @param[in] device DDF instance of the device to initialize.
 * @return Error code.
 */
static errno_t ehci_dev_add(ddf_dev_t *device)
{
	usb_log_debug("ehci_dev_add() called\n");
	assert(device);

	return hcd_ddf_add_hc(device, &ehci_hc_driver);

}


static const driver_ops_t ehci_driver_ops = {
	.dev_add = ehci_dev_add,
};

static const driver_t ehci_driver = {
	.name = NAME,
	.driver_ops = &ehci_driver_ops
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
	logctl_set_log_level(NAME, LVL_NOTE);
	return ddf_driver_main(&ehci_driver);
}

/**
 * @}
 */
