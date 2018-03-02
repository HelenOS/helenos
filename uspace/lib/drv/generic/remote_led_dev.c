/*
 * Copyright (c) 2014 Martin Decky
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

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#include <async.h>
#include <errno.h>
#include <io/pixel.h>
#include <macros.h>
#include <device/led_dev.h>
#include <ops/led_dev.h>
#include <ddf/driver.h>

static void remote_led_color_set(ddf_fun_t *, void *, ipc_callid_t,
    ipc_call_t *);

/** Remote LED interface operations */
static const remote_iface_func_ptr_t remote_led_dev_iface_ops[] = {
	[LED_DEV_COLOR_SET] = remote_led_color_set
};

/** Remote LED interface structure
 *
 * Interface for processing requests from remote clients
 * addressed by the LED interface.
 *
 */
const remote_iface_t remote_led_dev_iface = {
	.method_count = ARRAY_SIZE(remote_led_dev_iface_ops),
	.methods = remote_led_dev_iface_ops
};

/** Process the color_set() request from the remote client
 *
 * @param fun The function to which the data are written
 * @param ops The local ops structure
 *
 */
static void remote_led_color_set(ddf_fun_t *fun, void *ops, ipc_callid_t callid,
    ipc_call_t *call)
{
	led_dev_ops_t *led_dev_ops = (led_dev_ops_t *) ops;
	pixel_t color = DEV_IPC_GET_ARG1(*call);

	if (!led_dev_ops->color_set) {
		async_answer_0(callid, ENOTSUP);
		return;
	}

	errno_t rc = (*led_dev_ops->color_set)(fun, color);
	async_answer_0(callid, rc);
}

/**
 * @}
 */
