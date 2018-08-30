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

/** @addtogroup ps2mouse
 * @{
 */
/** @file
 * @brief PS/2 mouse driver
 */

#include <inttypes.h>
#include <ddf/driver.h>
#include <device/hw_res_parsed.h>
#include <errno.h>
#include <str_error.h>
#include <ddf/log.h>
#include <stdio.h>

#include "ps2mouse.h"

#define NAME "ps2mouse"

static errno_t mouse_add(ddf_dev_t *device);

/** DDF driver ops. */
static driver_ops_t mouse_driver_ops = {
	.dev_add = mouse_add,
};

/** DDF driver structure. */
static driver_t mouse_driver = {
	.name = NAME,
	.driver_ops = &mouse_driver_ops
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
	printf(NAME ": HelenOS ps/2 mouse driver.\n");
	ddf_log_init(NAME);
	return ddf_driver_main(&mouse_driver);
}

/** Initialize a new ddf driver instance
 *
 * @param[in] device DDF instance of the device to initialize.
 * @return Error code.
 */
static errno_t mouse_add(ddf_dev_t *device)
{
	errno_t rc;

	if (!device)
		return EINVAL;

	ps2_mouse_t *mouse = ddf_dev_data_alloc(device, sizeof(ps2_mouse_t));
	if (mouse == NULL) {
		ddf_msg(LVL_ERROR, "Failed to allocate mouse driver instance.");
		return ENOMEM;
	}

	rc = ps2_mouse_init(mouse, device);
	if (rc != EOK) {
		ddf_msg(LVL_ERROR, "Failed to initialize mouse driver: %s.",
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
