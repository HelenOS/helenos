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
/** @addtogroup drvkbd
 * @{
 */
/** @file
 * @brief XT keyboard driver
 */

#include <libarch/inttypes.h>
#include <ddf/driver.h>
#include <device/hw_res_parsed.h>
#include <errno.h>
#include <str_error.h>
#include <ddf/log.h>
#include <stdio.h>

#include "xtkbd.h"

#define NAME "xtkbd"

static int xt_kbd_add(ddf_dev_t *device);

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
 * @param[in] argc Nmber of arguments in argv vector (ignored).
 * @param[in] argv Cmdline argument vector (ignored).
 * @return Error code.
 *
 * Driver debug level is set here.
 */
int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS XT keyboard driver.\n");
	ddf_log_init(NAME, LVL_NOTE);
	return ddf_driver_main(&kbd_driver);
}

/** Initialize a new ddf driver instance of the driver
 *
 * @param[in] device DDF instance of the device to initialize.
 * @return Error code.
 */
static int xt_kbd_add(ddf_dev_t *device)
{
	if (!device)
		return EINVAL;

#define CHECK_RET_RETURN(ret, message...) \
if (ret != EOK) { \
	ddf_msg(LVL_ERROR, message); \
	return ret; \
} else (void)0

	xt_kbd_t *kbd = ddf_dev_data_alloc(device, sizeof(xt_kbd_t));
	int ret = (kbd == NULL) ? ENOMEM : EOK;
	CHECK_RET_RETURN(ret, "Failed to allocate XT/KBD driver instance.");

	ret = xt_kbd_init(kbd, device);
	CHECK_RET_RETURN(ret,
	    "Failed to initialize XT_KBD driver: %s.", str_error(ret));

	ddf_msg(LVL_NOTE, "Controlling '%s' (%" PRIun ").",
	    device->name, device->handle);
	return EOK;
}
/**
 * @}
 */
