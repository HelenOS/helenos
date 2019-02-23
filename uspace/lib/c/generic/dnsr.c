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
#include <errno.h>
#include <fibril_synch.h>
#include <inet/dnsr.h>
#include <ipc/dnsr.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdlib.h>
#include <str.h>

static FIBRIL_MUTEX_INITIALIZE(dnsr_sess_mutex);

static async_sess_t *dnsr_sess = NULL;

static async_exch_t *dnsr_exchange_begin(void)
{
	fibril_mutex_lock(&dnsr_sess_mutex);

	if (dnsr_sess == NULL) {
		service_id_t dnsr_svc;

		(void) loc_service_get_id(SERVICE_NAME_DNSR, &dnsr_svc,
		    IPC_FLAG_BLOCKING);

		dnsr_sess = loc_service_connect(dnsr_svc, INTERFACE_DNSR,
		    IPC_FLAG_BLOCKING);
	}

	async_sess_t *sess = dnsr_sess;
	fibril_mutex_unlock(&dnsr_sess_mutex);

	return async_exchange_begin(sess);
}

static void dnsr_exchange_end(async_exch_t *exch)
{
	async_exchange_end(exch);
}

errno_t dnsr_name2host(const char *name, dnsr_hostinfo_t **rinfo, ip_ver_t ver)
{
	async_exch_t *exch = dnsr_exchange_begin();

	ipc_call_t answer;
	aid_t req = async_send_1(exch, DNSR_NAME2HOST, (sysarg_t) ver,
	    &answer);

	errno_t rc = async_data_write_start(exch, name, str_size(name));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	dnsr_hostinfo_t *info = calloc(1, sizeof(dnsr_hostinfo_t));
	if (info == NULL)
		return ENOMEM;

	ipc_call_t answer_addr;
	aid_t req_addr = async_data_read(exch, &info->addr,
	    sizeof(inet_addr_t), &answer_addr);

	errno_t retval_addr;
	async_wait_for(req_addr, &retval_addr);

	if (retval_addr != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		free(info);
		return retval_addr;
	}

	ipc_call_t answer_cname;
	char cname_buf[DNSR_NAME_MAX_SIZE + 1];
	aid_t req_cname = async_data_read(exch, cname_buf, DNSR_NAME_MAX_SIZE,
	    &answer_cname);

	dnsr_exchange_end(exch);

	errno_t retval_cname;
	async_wait_for(req_cname, &retval_cname);

	if (retval_cname != EOK) {
		async_forget(req);
		free(info);
		return retval_cname;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	if (retval != EOK) {
		async_forget(req);
		free(info);
		return retval;
	}

	size_t act_size = ipc_get_arg2(&answer_cname);
	assert(act_size <= DNSR_NAME_MAX_SIZE);

	cname_buf[act_size] = '\0';

	info->cname = str_dup(cname_buf);

	if (info->cname == NULL) {
		free(info);
		return ENOMEM;
	}

	*rinfo = info;
	return EOK;
}

void dnsr_hostinfo_destroy(dnsr_hostinfo_t *info)
{
	if (info == NULL)
		return;

	free(info->cname);
	free(info);
}

errno_t dnsr_get_srvaddr(inet_addr_t *srvaddr)
{
	async_exch_t *exch = dnsr_exchange_begin();

	ipc_call_t answer;
	aid_t req = async_send_0(exch, DNSR_GET_SRVADDR, &answer);
	errno_t rc = async_data_read_start(exch, srvaddr, sizeof(inet_addr_t));

	loc_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	return retval;
}

errno_t dnsr_set_srvaddr(inet_addr_t *srvaddr)
{
	async_exch_t *exch = dnsr_exchange_begin();

	ipc_call_t answer;
	aid_t req = async_send_0(exch, DNSR_SET_SRVADDR, &answer);
	errno_t rc = async_data_write_start(exch, srvaddr, sizeof(inet_addr_t));

	loc_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	return retval;
}

/** @}
 */
