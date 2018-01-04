/*
 * Copyright (c) 2011 Vojtech Horky, Jan Vesely
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
 * @brief UHCI driver initialization
 */

#include <assert.h>
#include <ddf/driver.h>
#include <errno.h>
#include <io/log.h>
#include <io/logctl.h>
#include <pci_dev_iface.h>
#include <stdio.h>
#include <str_error.h>
#include <usb/debug.h>
#include <usb/host/ddf_helpers.h>

#include "hc.h"

#define NAME "uhci"

static errno_t uhci_driver_init(hcd_t *, const hw_res_list_parsed_t *, bool);
static void uhci_driver_fini(hcd_t *);
static errno_t disable_legacy(ddf_dev_t *);

static const ddf_hc_driver_t uhci_hc_driver = {
        .claim = disable_legacy,
        .hc_speed = USB_SPEED_FULL,
        .irq_code_gen = uhci_hc_gen_irq_code,
        .init = uhci_driver_init,
        .fini = uhci_driver_fini,
        .name = "UHCI",
	.ops = {
		.schedule    = uhci_hc_schedule,
		.irq_hook    = uhci_hc_interrupt,
		.status_hook = uhci_hc_status,
	},
};

static errno_t uhci_driver_init(hcd_t *hcd, const hw_res_list_parsed_t *res, bool irq)
{
	assert(hcd);
	assert(hcd_get_driver_data(hcd) == NULL);

	hc_t *instance = malloc(sizeof(hc_t));
	if (!instance)
		return ENOMEM;

	const errno_t ret = hc_init(instance, res, irq);
	if (ret == EOK) {
		hcd_set_implementation(hcd, instance, &uhci_hc_driver.ops);
	} else {
		free(instance);
	}
	return ret;
}

static void uhci_driver_fini(hcd_t *hcd)
{
	assert(hcd);
	hc_t *hc = hcd_get_driver_data(hcd);
	if (hc)
		hc_fini(hc);

	hcd_set_implementation(hcd, NULL, NULL);
	free(hc);
}

/** Call the PCI driver with a request to clear legacy support register
 *
 * @param[in] device Device asking to disable interrupts
 * @return Error code.
 */
static errno_t disable_legacy(ddf_dev_t *device)
{
	assert(device);

	async_sess_t *parent_sess = ddf_dev_parent_sess_get(device);
	if (parent_sess == NULL)
		return ENOMEM;

	/* See UHCI design guide page 45 for these values.
	 * Write all WC bits in USB legacy register */
	return pci_config_space_write_16(parent_sess, 0xc0, 0xaf00);
}

/** Initialize a new ddf driver instance for uhci hc and hub.
 *
 * @param[in] device DDF instance of the device to initialize.
 * @return Error code.
 */
static errno_t uhci_dev_add(ddf_dev_t *device)
{
	usb_log_debug2("uhci_dev_add() called\n");
	assert(device);
	return hcd_ddf_add_hc(device, &uhci_hc_driver);
}

static const driver_ops_t uhci_driver_ops = {
	.dev_add = uhci_dev_add,
};

static const driver_t uhci_driver = {
	.name = NAME,
	.driver_ops = &uhci_driver_ops
};


/** Initialize global driver structures (NONE).
 *
 * @param[in] argc Number of arguments in argv vector (ignored).
 * @param[in] argv Cmdline argument vector (ignored).
 * @return Error code.
 *
 * Driver debug level is set here.
 */
int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS UHCI driver.\n");
	log_init(NAME);
	logctl_set_log_level(NAME, LVL_NOTE);
	return ddf_driver_main(&uhci_driver);
}
/**
 * @}
 */
