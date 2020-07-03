/*
 * Copyright (c) 2019 Jiri Svoboda
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

#include <async.h>
#include <ddev.h>
#include <errno.h>
#include <ipc/ddev.h>
#include <ipc/services.h>
#include <ipcgfx/client.h>
#include <loc.h>
#include <stdlib.h>

/** Open display device.
 *
 * @param ddname Display device service name
 * @param rdisplay Place to store pointer to display session
 * @return EOK on success or an error code
 */
errno_t ddev_open(const char *ddname, ddev_t **rddev)
{
	service_id_t ddev_svc;
	ddev_t *ddev;
	errno_t rc;

	ddev = calloc(1, sizeof(ddev_t));
	if (ddev == NULL)
		return ENOMEM;

	rc = loc_service_get_id(ddname, &ddev_svc, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		free(ddev);
		return ENOENT;
	}

	ddev->sess = loc_service_connect(ddev_svc, INTERFACE_DDEV,
	    IPC_FLAG_BLOCKING);
	if (ddev->sess == NULL) {
		free(ddev);
		return ENOENT;
	}

	*rddev = ddev;
	return EOK;
}

/** Close display device.
 *
 * @param ddev Display device session
 */
void ddev_close(ddev_t *ddev)
{
	async_hangup(ddev->sess);
	free(ddev);
}

/** Create graphics context for drawing to display device.
 *
 * @param ddev Display device
 * @param rgc Place to store pointer to new graphics context
 */
errno_t ddev_get_gc(ddev_t *ddev, gfx_context_t **rgc)
{
	async_sess_t *sess;
	async_exch_t *exch;
	sysarg_t arg2;
	sysarg_t arg3;
	ipc_gc_t *gc;
	errno_t rc;

	exch = async_exchange_begin(ddev->sess);
	rc = async_req_0_2(exch, DDEV_GET_GC, &arg2, &arg3);
	if (rc != EOK) {
		async_exchange_end(exch);
		return rc;
	}

	sess = async_connect_me_to(exch, INTERFACE_GC, arg2, arg3, &rc);
	async_exchange_end(exch);

	if (sess == NULL)
		return rc;

	rc = ipc_gc_create(sess, &gc);
	if (rc != EOK) {
		async_hangup(sess);
		return ENOMEM;
	}

	*rgc = ipc_gc_get_ctx(gc);
	return EOK;
}

/** Get display device information.
 *
 * @param ddev Display device
 * @param info Place to store information
 */
errno_t ddev_get_info(ddev_t *ddev, ddev_info_t *info)
{
	async_exch_t *exch;
	errno_t retval;
	ipc_call_t answer;

	exch = async_exchange_begin(ddev->sess);
	aid_t req = async_send_0(exch, DDEV_GET_INFO, &answer);

	errno_t rc = async_data_read_start(exch, info, sizeof(ddev_info_t));
	async_exchange_end(exch);
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	async_wait_for(req, &retval);
	if (retval != EOK)
		return rc;

	return EOK;
}

/** @}
 */
