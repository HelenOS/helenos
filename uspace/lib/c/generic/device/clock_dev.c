/*
 * Copyright (c) 2012 Maurizio Lombardi
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <ipc/dev_iface.h>
#include <device/clock_dev.h>
#include <errno.h>
#include <async.h>
#include <time.h>

/** Read the current time from the device
 *
 * @param sess     Session of the device
 * @param t        The current time that will be read from the device
 *
 * @return         EOK on success or an error code
 */
errno_t
clock_dev_time_get(async_sess_t *sess, struct tm *t)
{
	aid_t req;
	errno_t ret;

	async_exch_t *exch = async_exchange_begin(sess);

	req = async_send_1(exch, DEV_IFACE_ID(CLOCK_DEV_IFACE),
	    CLOCK_DEV_TIME_GET, NULL);
	ret = async_data_read_start(exch, t, sizeof(*t));

	async_exchange_end(exch);

	errno_t rc;
	if (ret != EOK) {
		async_forget(req);
		return ret;
	}

	async_wait_for(req, &rc);
	return (errno_t)rc;
}

/** Set the current time
 *
 * @param sess   Session of the device
 * @param t      The current time that will be written to the device
 *
 * @return       EOK on success or an error code
 */
errno_t
clock_dev_time_set(async_sess_t *sess, struct tm *t)
{
	aid_t req;
	errno_t ret;

	async_exch_t *exch = async_exchange_begin(sess);

	req = async_send_1(exch, DEV_IFACE_ID(CLOCK_DEV_IFACE),
	    CLOCK_DEV_TIME_SET, NULL);
	ret = async_data_write_start(exch, t, sizeof(*t));

	async_exchange_end(exch);

	errno_t rc;
	if (ret != EOK) {
		async_forget(req);
		return ret;
	}

	async_wait_for(req, &rc);
	return (errno_t)rc;
}

/** @}
 */

