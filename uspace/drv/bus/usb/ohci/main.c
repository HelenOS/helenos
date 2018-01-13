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

/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * Main routines of OHCI driver.
 */

#include <assert.h>
#include <ddf/driver.h>
#include <errno.h>
#include <io/log.h>
#include <str_error.h>

#include <usb/debug.h>
#include <usb/host/ddf_helpers.h>

#include "hc.h"

#define NAME "ohci"
static errno_t ohci_driver_init(hcd_t *, const hw_res_list_parsed_t *, bool);
static void ohci_driver_fini(hcd_t *);

static const ddf_hc_driver_t ohci_hc_driver = {
        .hc_speed = USB_SPEED_FULL,
        .irq_code_gen = ohci_hc_gen_irq_code,
        .init = ohci_driver_init,
        .fini = ohci_driver_fini,
        .name = "OHCI",
	.ops = {
                .schedule       = ohci_hc_schedule,
                .ep_add_hook    = ohci_endpoint_init,
                .ep_remove_hook = ohci_endpoint_fini,
                .irq_hook       = ohci_hc_interrupt,
                .status_hook    = ohci_hc_status,
	},
};


static errno_t ohci_driver_init(hcd_t *hcd, const hw_res_list_parsed_t *res, bool irq)
{
	assert(hcd);
	assert(hcd_get_driver_data(hcd) == NULL);

	hc_t *instance = malloc(sizeof(hc_t));
	if (!instance)
		return ENOMEM;

	const errno_t ret = hc_init(instance, res, irq);
	if (ret == EOK) {
		hcd_set_implementation(hcd, instance, &ohci_hc_driver.ops);
	} else {
		free(instance);
	}
	return ret;
}

static void ohci_driver_fini(hcd_t *hcd)
{
	assert(hcd);
	hc_t *hc = hcd_get_driver_data(hcd);
	if (hc)
		hc_fini(hc);

	hcd_set_implementation(hcd, NULL, NULL);
	free(hc);
}

/** Initializes a new ddf driver instance of OHCI hcd.
 *
 * @param[in] device DDF instance of the device to initialize.
 * @return Error code.
 */
static errno_t ohci_dev_add(ddf_dev_t *device)
{
	usb_log_debug("ohci_dev_add() called\n");
	assert(device);
	return hcd_ddf_add_hc(device, &ohci_hc_driver);
}

static const driver_ops_t ohci_driver_ops = {
	.dev_add = ohci_dev_add,
};

static const driver_t ohci_driver = {
	.name = NAME,
	.driver_ops = &ohci_driver_ops
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
	return ddf_driver_main(&ohci_driver);
}

/**
 * @}
 */
