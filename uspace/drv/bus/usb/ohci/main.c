/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 * SPDX-FileCopyrightText: 2018 Ondrej Hlavaty, Petr Manek
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <io/logctl.h>
#include <str_error.h>

#include <usb/debug.h>

#include "hc.h"
#include "ohci_bus.h"

#define NAME "ohci"

static const hc_driver_t ohci_driver = {
	.name = NAME,
	.hc_device_size = sizeof(hc_t),

	.hc_add = hc_add,
	.irq_code_gen = hc_gen_irq_code,
	.claim = hc_gain_control,
	.start = hc_start,
	.setup_root_hub = hc_setup_roothub,
	.hc_gone = hc_gone,
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
	return hc_driver_main(&ohci_driver);
}

/**
 * @}
 */
