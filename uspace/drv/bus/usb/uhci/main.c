/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky, Jan Vesely
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty, Petr Manek
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <usb/host/utility.h>

#include "hc.h"

#define NAME "uhci"

static errno_t disable_legacy(hc_device_t *);

static const hc_driver_t uhci_driver = {
	.name = NAME,
	.hc_device_size = sizeof(hc_t),
	.claim = disable_legacy,
	.irq_code_gen = hc_gen_irq_code,
	.hc_add = hc_add,
	.start = hc_start,
	.setup_root_hub = hc_setup_roothub,
	.hc_gone = hc_gone,
};

/** Call the PCI driver with a request to clear legacy support register
 *
 * @param[in] device Device asking to disable interrupts
 * @return Error code.
 */
static errno_t disable_legacy(hc_device_t *hcd)
{
	assert(hcd);

	async_sess_t *parent_sess = ddf_dev_parent_sess_get(hcd->ddf_dev);
	if (parent_sess == NULL)
		return ENOMEM;

	/*
	 * See UHCI design guide page 45 for these values.
	 * Write all WC bits in USB legacy register
	 */
	return pci_config_space_write_16(parent_sess, 0xc0, 0xaf00);
}

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
	return hc_driver_main(&uhci_driver);
}
/**
 * @}
 */
