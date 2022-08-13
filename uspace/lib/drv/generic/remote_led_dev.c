/*
 * SPDX-FileCopyrightText: 2014 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

static void remote_led_color_set(ddf_fun_t *, void *, ipc_call_t *);

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
static void remote_led_color_set(ddf_fun_t *fun, void *ops, ipc_call_t *call)
{
	led_dev_ops_t *led_dev_ops = (led_dev_ops_t *) ops;
	pixel_t color = DEV_IPC_GET_ARG1(*call);

	if (!led_dev_ops->color_set) {
		async_answer_0(call, ENOTSUP);
		return;
	}

	errno_t rc = (*led_dev_ops->color_set)(fun, color);
	async_answer_0(call, rc);
}

/**
 * @}
 */
