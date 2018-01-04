/*
 * Copyright (c) 2013 Jiri Svoboda
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
#include <assert.h>
#include <corecfg.h>
#include <errno.h>
#include <ipc/corecfg.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdlib.h>

static async_sess_t *corecfg_sess = NULL;

errno_t corecfg_init(void)
{
	service_id_t corecfg_svc;
	errno_t rc;

	assert(corecfg_sess == NULL);

	rc = loc_service_get_id(SERVICE_NAME_CORECFG, &corecfg_svc,
	    IPC_FLAG_BLOCKING);
	if (rc != EOK)
		return ENOENT;

	corecfg_sess = loc_service_connect(corecfg_svc, INTERFACE_CORECFG,
	    IPC_FLAG_BLOCKING);
	if (corecfg_sess == NULL)
		return ENOENT;

	return EOK;
}

/** Get core dump enable status. */
errno_t corecfg_get_enable(bool *renable)
{
	async_exch_t *exch = async_exchange_begin(corecfg_sess);
	sysarg_t enable;

	errno_t rc = async_req_0_1(exch, CORECFG_GET_ENABLE, &enable);

	async_exchange_end(exch);

	if (rc != EOK)
		return rc;

	*renable = enable;
	return EOK;
}

/** Enable or disable core dumps. */
errno_t corecfg_set_enable(bool enable)
{
	async_exch_t *exch = async_exchange_begin(corecfg_sess);
	errno_t rc = async_req_1_0(exch, CORECFG_SET_ENABLE, (sysarg_t) enable);

	async_exchange_end(exch);

	if (rc != EOK)
		return rc;

	return EOK;
}

/** @}
 */
