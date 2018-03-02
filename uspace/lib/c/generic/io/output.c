/*
 * Copyright (c) 2011 Martin Decky
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

#include <async.h>
#include <errno.h>
#include <as.h>
#include <ipc/output.h>
#include <io/concaps.h>
#include <io/output.h>

errno_t output_yield(async_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t ret = async_req_0_0(exch, OUTPUT_YIELD);
	async_exchange_end(exch);

	return ret;
}

errno_t output_claim(async_sess_t *sess)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t ret = async_req_0_0(exch, OUTPUT_CLAIM);
	async_exchange_end(exch);

	return ret;
}

errno_t output_get_dimensions(async_sess_t *sess, sysarg_t *maxx, sysarg_t *maxy)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t ret = async_req_0_2(exch, OUTPUT_GET_DIMENSIONS, maxx, maxy);
	async_exchange_end(exch);

	return ret;
}

errno_t output_get_caps(async_sess_t *sess, console_caps_t *ccaps)
{
	async_exch_t *exch = async_exchange_begin(sess);

	sysarg_t rv;
	errno_t ret = async_req_0_1(exch, OUTPUT_GET_CAPS, &rv);

	async_exchange_end(exch);

	if (ret == EOK)
		*ccaps = (console_caps_t) rv;

	return ret;
}

frontbuf_handle_t output_frontbuf_create(async_sess_t *sess,
    chargrid_t *frontbuf)
{
	async_exch_t *exch = async_exchange_begin(sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, OUTPUT_FRONTBUF_CREATE, &answer);
	errno_t rc = async_share_out_start(exch, frontbuf, AS_AREA_READ
	    | AS_AREA_WRITE | AS_AREA_CACHEABLE);

	async_exchange_end(exch);

	errno_t ret;
	async_wait_for(req, &ret);

	if ((rc != EOK) || (ret != EOK))
		return 0;

	return (frontbuf_handle_t) IPC_GET_ARG1(answer);
}

errno_t output_set_style(async_sess_t *sess, console_style_t style)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t ret = async_req_1_0(exch, OUTPUT_SET_STYLE, style);
	async_exchange_end(exch);

	return ret;
}

errno_t output_cursor_update(async_sess_t *sess, frontbuf_handle_t frontbuf)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t ret = async_req_1_0(exch, OUTPUT_CURSOR_UPDATE, frontbuf);
	async_exchange_end(exch);

	return ret;
}

errno_t output_update(async_sess_t *sess, frontbuf_handle_t frontbuf)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t ret = async_req_1_0(exch, OUTPUT_UPDATE, frontbuf);
	async_exchange_end(exch);

	return ret;
}

errno_t output_damage(async_sess_t *sess, frontbuf_handle_t frontbuf, sysarg_t col,
    sysarg_t row, sysarg_t cols, sysarg_t rows)
{
	async_exch_t *exch = async_exchange_begin(sess);
	errno_t ret = async_req_5_0(exch, OUTPUT_DAMAGE, frontbuf, col, row,
	    cols, rows);
	async_exchange_end(exch);

	return ret;
}

/** @}
 */
