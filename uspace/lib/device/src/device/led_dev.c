/*
 * SPDX-FileCopyrightText: 2014 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 */

#include <errno.h>
#include <async.h>
#include <io/pixel.h>
#include <ipc/dev_iface.h>
#include <device/led_dev.h>

errno_t led_dev_color_set(async_sess_t *sess, pixel_t pixel)
{
	async_exch_t *exch = async_exchange_begin(sess);

	aid_t req = async_send_2(exch, DEV_IFACE_ID(LED_DEV_IFACE),
	    LED_DEV_COLOR_SET, (sysarg_t) pixel, NULL);

	async_exchange_end(exch);

	errno_t rc;
	async_wait_for(req, &rc);

	return (errno_t) rc;
}

/** @}
 */
