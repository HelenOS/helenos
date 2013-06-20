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
		
		dnsr_sess = loc_service_connect(EXCHANGE_SERIALIZE, dnsr_svc,
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

int dnsr_name2host(const char *name, dnsr_hostinfo_t **rinfo)
{
	async_exch_t *exch = dnsr_exchange_begin();
	char cname_buf[DNSR_NAME_MAX_SIZE + 1];
	ipc_call_t cnreply;
	size_t act_size;
	dnsr_hostinfo_t *info;

	ipc_call_t answer;
	aid_t req = async_send_0(exch, DNSR_NAME2HOST, &answer);
	sysarg_t retval = async_data_write_start(exch, name, str_size(name));
	aid_t cnreq = async_data_read(exch, cname_buf, DNSR_NAME_MAX_SIZE,
	    &cnreply);

	dnsr_exchange_end(exch);

	if (retval != EOK) {
		async_forget(req);
		async_forget(cnreq);
		return retval;
	}

	async_wait_for(req, &retval);
	if (retval != EOK) {
		async_forget(cnreq);
		return EIO;
	}

	async_wait_for(cnreq, &retval);
	if (retval != EOK)
		return EIO;

	info = calloc(1, sizeof(dnsr_hostinfo_t));
	if (info == NULL)
		return ENOMEM;

	act_size = IPC_GET_ARG2(cnreply);
	assert(act_size <= DNSR_NAME_MAX_SIZE);
	cname_buf[act_size] = '\0';

	info->cname = str_dup(cname_buf);
	inet2_addr_unpack(IPC_GET_ARG1(answer), &info->addr);

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

int dnsr_get_srvaddr(inet2_addr_t *srvaddr)
{
	async_exch_t *exch = dnsr_exchange_begin();
	
	ipc_call_t answer;
	aid_t req = async_send_0(exch, DNSR_GET_SRVADDR, &answer);
	int rc = async_data_read_start(exch, srvaddr, sizeof(inet2_addr_t));
	
	loc_exchange_end(exch);
	
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}
	
	sysarg_t retval;
	async_wait_for(req, &retval);
	
	return (int) retval;
}

int dnsr_set_srvaddr(inet2_addr_t *srvaddr)
{
	async_exch_t *exch = dnsr_exchange_begin();
	
	ipc_call_t answer;
	aid_t req = async_send_0(exch, DNSR_SET_SRVADDR, &answer);
	int rc = async_data_write_start(exch, srvaddr, sizeof(inet2_addr_t));
	
	loc_exchange_end(exch);
	
	if (rc != EOK) {
		async_forget(req);
		return rc;
	}
	
	sysarg_t retval;
	async_wait_for(req, &retval);
	
	return (int) retval;
}

/** @}
 */
