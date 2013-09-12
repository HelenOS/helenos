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
#include <stdbool.h>
#include <str_error.h>

#include <usb_iface.h>
#include <usb/ddfiface.h>
#include <usb/debug.h>
#include <usb/host/hcd.h>

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
static ddf_dev_ops_t hc_ops = {
	.interfaces[USBHC_DEV_IFACE] = &hcd_iface,
};


/** Initializes a new ddf driver instance of EHCI hcd.
 *
 * @param[in] device DDF instance of the device to initialize.
 * @return Error code.
 */
static int ehci_dev_add(ddf_dev_t *device)
{
	ddf_fun_t *hc_fun = NULL;
	bool fun_bound = false;

	assert(device);

	addr_range_t reg_range;
	int irq = 0;

	int rc = get_my_registers(device, &reg_range, &irq);
	if (rc != EOK) {
		usb_log_error("Failed to get memory addresses for %" PRIun
		    ": %s.\n", ddf_dev_get_handle(device), str_error(rc));
		goto error;
	}

	usb_log_info("Memory mapped regs at %p (size %zu), IRQ %d.\n",
	    RNGABSPTR(reg_range), RNGSZ(reg_range), irq);

	rc = disable_legacy(device, &reg_range);
	if (rc != EOK) {
		usb_log_error("Failed to disable legacy USB: %s.\n",
		    str_error(rc));
		goto error;
	}

	hc_fun = ddf_fun_create(device, fun_exposed, "ehci_hc");
	if (hc_fun == NULL) {
		usb_log_error("Failed to create EHCI function.\n");
		rc = ENOMEM;
		goto error;
	}

	hcd_t *ehci_hc = ddf_fun_data_alloc(hc_fun, sizeof(hcd_t));
	if (ehci_hc == NULL) {
		usb_log_error("Failed to alloc generic HC driver.\n");
		rc = ENOMEM;
		goto error;
	}

	/* High Speed, no bandwidth */
	hcd_init(ehci_hc, USB_SPEED_HIGH, 0, NULL);
	ddf_fun_set_ops(hc_fun,  &hc_ops);

	rc = ddf_fun_bind(hc_fun);
	if (rc != EOK) {
		usb_log_error("Failed to bind EHCI function: %s.\n",
		    str_error(rc));
		goto error;
	}

	fun_bound = true;

	rc = ddf_fun_add_to_category(hc_fun, USB_HC_CATEGORY);
	if (rc != EOK) {
		usb_log_error("Failed to add EHCI to HC class: %s.\n",
		    str_error(rc));
		goto error;
	}

	usb_log_info("Controlling new EHCI device `%s' (handle %" PRIun ").\n",
	    ddf_dev_get_name(device), ddf_dev_get_handle(device));

	return EOK;
error:
	if (fun_bound)
		ddf_fun_unbind(hc_fun);
	if (hc_fun != NULL)
		ddf_fun_destroy(hc_fun);
	return rc;
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
