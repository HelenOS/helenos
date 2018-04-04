/*
 * Copyright (c) 2011 Petr Koupy
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

#include <assert.h>
#include <errno.h>
#include <as.h>
#include <ipc/graph.h>
#include <io/visualizer.h>

errno_t visualizer_claim(async_sess_t *sess, sysarg_t notif_callback_id)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t ret = async_req_1_0(exch, VISUALIZER_CLAIM, notif_callback_id);
	async_exchange_end(exch);

	return ret;
}

errno_t visualizer_yield(async_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t ret = async_req_0_0(exch, VISUALIZER_YIELD);
	async_exchange_end(exch);

	return ret;
}

errno_t visualizer_enumerate_modes(async_sess_t *sess, vslmode_t *mode, sysarg_t nth)
{
	async_exch_t *exch = async_exchange_begin(sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, VISUALIZER_ENUMERATE_MODES, nth, &answer);

	errno_t rc = async_data_read_start(exch, mode, sizeof(vslmode_t));

	async_exchange_end(exch);

	errno_t ret;
	async_wait_for(req, &ret);

	if (rc != EOK) {
		return rc;
	} else if (ret != EOK) {
		return ret;
	} else {
		return EOK;
	}
}

errno_t visualizer_get_default_mode(async_sess_t *sess, vslmode_t *mode)
{
	async_exch_t *exch = async_exchange_begin(sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, VISUALIZER_GET_DEFAULT_MODE, &answer);

	errno_t rc = async_data_read_start(exch, mode, sizeof(vslmode_t));

	async_exchange_end(exch);

	errno_t ret;
	async_wait_for(req, &ret);

	if (rc != EOK) {
		return rc;
	} else if (ret != EOK) {
		return ret;
	} else {
		return EOK;
	}
}

errno_t visualizer_get_current_mode(async_sess_t *sess, vslmode_t *mode)
{
	async_exch_t *exch = async_exchange_begin(sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, VISUALIZER_GET_CURRENT_MODE, &answer);

	errno_t rc = async_data_read_start(exch, mode, sizeof(vslmode_t));

	async_exchange_end(exch);

	errno_t ret;
	async_wait_for(req, &ret);

	if (rc != EOK) {
		return rc;
	} else if (ret != EOK) {
		return ret;
	} else {
		return EOK;
	}
}

errno_t visualizer_get_mode(async_sess_t *sess, vslmode_t *mode, sysarg_t index)
{
	async_exch_t *exch = async_exchange_begin(sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, VISUALIZER_GET_MODE, index, &answer);

	errno_t rc = async_data_read_start(exch, mode, sizeof(vslmode_t));

	async_exchange_end(exch);

	errno_t ret;
	async_wait_for(req, &ret);

	if (rc != EOK) {
		return rc;
	} else if (ret != EOK) {
		return ret;
	} else {
		return EOK;
	}
}

errno_t visualizer_set_mode(async_sess_t *sess, sysarg_t index, sysarg_t version, void *cells)
{
	async_exch_t *exch = async_exchange_begin(sess);

	ipc_call_t answer;
	aid_t req = async_send_2(exch, VISUALIZER_SET_MODE, index, version, &answer);

	errno_t rc = async_share_out_start(exch, cells, AS_AREA_READ | AS_AREA_CACHEABLE);

	async_exchange_end(exch);

	errno_t ret;
	async_wait_for(req, &ret);

	if (rc != EOK) {
		return rc;
	} else if (ret != EOK) {
		return ret;
	} else {
		return EOK;
	}
}

errno_t visualizer_update_damaged_region(async_sess_t *sess,
    sysarg_t x, sysarg_t y, sysarg_t width, sysarg_t height,
    sysarg_t x_offset, sysarg_t y_offset)
{
	assert(x_offset <= UINT16_MAX);
	assert(y_offset <= UINT16_MAX);

	sysarg_t offsets = ((x_offset << 16) | (y_offset & 0x0000ffff));

	async_exch_t *exch = async_exchange_begin(sess);
	errno_t ret = async_req_5_0(exch, VISUALIZER_UPDATE_DAMAGED_REGION,
	    x, y, width, height, offsets);
	async_exchange_end(exch);

	return ret;
}

errno_t visualizer_suspend(async_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t ret = async_req_0_0(exch, VISUALIZER_SUSPEND);
	async_exchange_end(exch);

	return ret;
}

errno_t visualizer_wakeup(async_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t ret = async_req_0_0(exch, VISUALIZER_WAKE_UP);
	async_exchange_end(exch);

	return ret;
}

/** @}
 */
