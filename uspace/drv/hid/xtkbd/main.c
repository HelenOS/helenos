/*
 * SPDX-FileCopyrightText: 2011 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup xtkbd
 * @{
 */
/** @file
 * @brief XT keyboard driver
 */

#include <inttypes.h>
#include <ddf/driver.h>
#include <device/hw_res_parsed.h>
#include <errno.h>
#include <str_error.h>
#include <ddf/log.h>
#include <stdio.h>

#include "xtkbd.h"

#define NAME "xtkbd"

static errno_t xt_kbd_add(ddf_dev_t *device);

/** DDF driver ops. */
static driver_ops_t kbd_driver_ops = {
	.dev_add = xt_kbd_add,
};

/** DDF driver structure. */
static driver_t kbd_driver = {
	.name = NAME,
	.driver_ops = &kbd_driver_ops
};

/** Initialize global driver structures (NONE).
 *
 * Driver debug level is set here.
 *
 * @param[in] argc Nmber of arguments in argv vector (ignored).
 * @param[in] argv Cmdline argument vector (ignored).
 *
 * @return Error code.
 *
 */
int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS XT keyboard driver.\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&kbd_driver);
}

/** Initialize a new ddf driver instance of the driver
 *
 * @param[in] device DDF instance of the device to initialize.
 *
 * @return Error code.
 *
 */
static errno_t xt_kbd_add(ddf_dev_t *device)
{
	errno_t rc;

	if (!device)
		return EINVAL;

	xt_kbd_t *kbd = ddf_dev_data_alloc(device, sizeof(xt_kbd_t));
	if (kbd == NULL) {
		ddf_msg(LVL_ERROR, "Failed to allocate XT/KBD driver instance.");
		return ENOMEM;
	}

	rc = xt_kbd_init(kbd, device);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to initialize XT_KBD driver: %s.",
		    str_error(rc));
		return rc;
	}

	ddf_msg(LVL_NOTE, "Controlling '%s' (%" PRIun ").",
	    ddf_dev_get_name(device), ddf_dev_get_handle(device));
	return EOK;
}

/**
 * @}
 */
