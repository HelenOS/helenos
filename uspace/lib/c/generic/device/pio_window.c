/*
 * SPDX-FileCopyrightText: 2013 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <device/pio_window.h>
#include <errno.h>
#include <async.h>

errno_t pio_window_get(async_sess_t *sess, pio_window_t *pio_win)
{
	errno_t rc;

	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return ENOMEM;

	rc = async_req_1_0(exch, DEV_IFACE_ID(PIO_WINDOW_DEV_IFACE),
	    PIO_WINDOW_GET);
	if (rc != EOK) {
		async_exchange_end(exch);
		return rc;
	}

	rc = async_data_read_start(exch, pio_win, sizeof(*pio_win));
	async_exchange_end(exch);

	return rc;
}

/** @}
 */
