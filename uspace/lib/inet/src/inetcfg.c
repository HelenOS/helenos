/*
 * Copyright (c) 2021 Jiri Svoboda
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
#include <inet/eth_addr.h>
#include <inet/inetcfg.h>
#include <ipc/inet.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdlib.h>
#include <str.h>

static async_sess_t *inetcfg_sess = NULL;

static errno_t inetcfg_get_ids_once(sysarg_t method, sysarg_t arg1,
    sysarg_t *id_buf, size_t buf_size, size_t *act_size)
{
	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, method, arg1, &answer);
	errno_t rc = async_data_read_start(exch, id_buf, buf_size);

	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	if (retval != EOK) {
		return retval;
	}

	*act_size = ipc_get_arg1(&answer);
	return EOK;
}

/** Get list of IDs.
 *
 * Returns an allocated array of service IDs.
 *
 * @param method	IPC method
 * @param arg1		IPC argument 1
 * @param data		Place to store pointer to array of IDs
 * @param count		Place to store number of IDs
 * @return 		EOK on success or an error code
 */
static errno_t inetcfg_get_ids_internal(sysarg_t method, sysarg_t arg1,
    sysarg_t **data, size_t *count)
{
	*data = NULL;
	*count = 0;

	size_t act_size = 0;
	errno_t rc = inetcfg_get_ids_once(method, arg1, NULL, 0,
	    &act_size);
	if (rc != EOK)
		return rc;

	size_t alloc_size = act_size;
	service_id_t *ids = malloc(alloc_size);
	if (ids == NULL)
		return ENOMEM;

	while (true) {
		rc = inetcfg_get_ids_once(method, arg1, ids, alloc_size,
		    &act_size);
		if (rc != EOK)
			return rc;

		if (act_size <= alloc_size)
			break;

		alloc_size = act_size;
		service_id_t *tmp = realloc(ids, alloc_size);
		if (tmp == NULL) {
			free(ids);
			return ENOMEM;
		}
		ids = tmp;
	}

	*count = act_size / sizeof(sysarg_t);
	*data = ids;
	return EOK;
}

errno_t inetcfg_init(void)
{
	service_id_t inet_svc;
	errno_t rc;

	assert(inetcfg_sess == NULL);

	rc = loc_service_get_id(SERVICE_NAME_INET, &inet_svc,
	    IPC_FLAG_BLOCKING);
	if (rc != EOK)
		return ENOENT;

	inetcfg_sess = loc_service_connect(inet_svc, INTERFACE_INETCFG,
	    IPC_FLAG_BLOCKING);
	if (inetcfg_sess == NULL)
		return ENOENT;

	return EOK;
}

errno_t inetcfg_addr_create_static(const char *name, inet_naddr_t *naddr,
    sysarg_t link_id, sysarg_t *addr_id)
{
	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, INETCFG_ADDR_CREATE_STATIC, link_id,
	    &answer);

	errno_t rc = async_data_write_start(exch, naddr, sizeof(inet_naddr_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	rc = async_data_write_start(exch, name, str_size(name));

	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	*addr_id = ipc_get_arg1(&answer);

	return retval;
}

errno_t inetcfg_addr_delete(sysarg_t addr_id)
{
	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	errno_t rc = async_req_1_0(exch, INETCFG_ADDR_DELETE, addr_id);
	async_exchange_end(exch);

	return rc;
}

errno_t inetcfg_addr_get(sysarg_t addr_id, inet_addr_info_t *ainfo)
{
	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, INETCFG_ADDR_GET, addr_id, &answer);

	ipc_call_t answer_naddr;
	aid_t req_naddr = async_data_read(exch, &ainfo->naddr,
	    sizeof(inet_naddr_t), &answer_naddr);

	errno_t retval_naddr;
	async_wait_for(req_naddr, &retval_naddr);

	if (retval_naddr != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return retval_naddr;
	}

	ipc_call_t answer_name;
	char name_buf[LOC_NAME_MAXLEN + 1];
	aid_t req_name = async_data_read(exch, name_buf, LOC_NAME_MAXLEN,
	    &answer_name);

	async_exchange_end(exch);

	errno_t retval_name;
	async_wait_for(req_name, &retval_name);

	if (retval_name != EOK) {
		async_forget(req);
		return retval_name;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	if (retval != EOK)
		return retval;

	size_t act_size = ipc_get_arg2(&answer_name);
	assert(act_size <= LOC_NAME_MAXLEN);

	name_buf[act_size] = '\0';

	ainfo->ilink = ipc_get_arg1(&answer);
	ainfo->name = str_dup(name_buf);

	return EOK;
}

errno_t inetcfg_addr_get_id(const char *name, sysarg_t link_id, sysarg_t *addr_id)
{
	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, INETCFG_ADDR_GET_ID, link_id, &answer);
	errno_t retval = async_data_write_start(exch, name, str_size(name));

	async_exchange_end(exch);

	if (retval != EOK) {
		async_forget(req);
		return retval;
	}

	async_wait_for(req, &retval);
	*addr_id = ipc_get_arg1(&answer);

	return retval;
}

errno_t inetcfg_get_addr_list(sysarg_t **addrs, size_t *count)
{
	return inetcfg_get_ids_internal(INETCFG_GET_ADDR_LIST,
	    0, addrs, count);
}

errno_t inetcfg_get_link_list(sysarg_t **links, size_t *count)
{
	return inetcfg_get_ids_internal(INETCFG_GET_LINK_LIST,
	    0, links, count);
}

errno_t inetcfg_get_sroute_list(sysarg_t **sroutes, size_t *count)
{
	return inetcfg_get_ids_internal(INETCFG_GET_SROUTE_LIST,
	    0, sroutes, count);
}

errno_t inetcfg_link_add(sysarg_t link_id)
{
	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	errno_t rc = async_req_1_0(exch, INETCFG_LINK_ADD, link_id);
	async_exchange_end(exch);

	return rc;
}

errno_t inetcfg_link_get(sysarg_t link_id, inet_link_info_t *linfo)
{
	ipc_call_t dreply;
	errno_t dretval;
	size_t act_size;
	char name_buf[LOC_NAME_MAXLEN + 1];

	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, INETCFG_LINK_GET, link_id, &answer);
	aid_t dreq = async_data_read(exch, name_buf, LOC_NAME_MAXLEN, &dreply);
	errno_t rc = async_data_read_start(exch, &linfo->mac_addr, sizeof(eth_addr_t));
	async_wait_for(dreq, &dretval);

	async_exchange_end(exch);

	if (dretval != EOK || rc != EOK) {
		async_forget(req);
		return dretval;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	if (retval != EOK)
		return retval;

	act_size = ipc_get_arg2(&dreply);
	assert(act_size <= LOC_NAME_MAXLEN);
	name_buf[act_size] = '\0';

	linfo->name = str_dup(name_buf);
	linfo->def_mtu = ipc_get_arg1(&answer);

	return EOK;
}

errno_t inetcfg_link_remove(sysarg_t link_id)
{
	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	errno_t rc = async_req_1_0(exch, INETCFG_LINK_REMOVE, link_id);
	async_exchange_end(exch);

	return rc;
}

errno_t inetcfg_sroute_create(const char *name, inet_naddr_t *dest,
    inet_addr_t *router, sysarg_t *sroute_id)
{
	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, INETCFG_SROUTE_CREATE, &answer);

	errno_t rc = async_data_write_start(exch, dest, sizeof(inet_naddr_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	rc = async_data_write_start(exch, router, sizeof(inet_addr_t));
	if (rc != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return rc;
	}

	rc = async_data_write_start(exch, name, str_size(name));

	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	*sroute_id = ipc_get_arg1(&answer);

	return retval;
}

errno_t inetcfg_sroute_delete(sysarg_t sroute_id)
{
	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	errno_t rc = async_req_1_0(exch, INETCFG_SROUTE_DELETE, sroute_id);
	async_exchange_end(exch);

	return rc;
}

errno_t inetcfg_sroute_get(sysarg_t sroute_id, inet_sroute_info_t *srinfo)
{
	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	ipc_call_t answer;
	aid_t req = async_send_1(exch, INETCFG_SROUTE_GET, sroute_id, &answer);

	ipc_call_t answer_dest;
	aid_t req_dest = async_data_read(exch, &srinfo->dest,
	    sizeof(inet_naddr_t), &answer_dest);

	errno_t retval_dest;
	async_wait_for(req_dest, &retval_dest);

	if (retval_dest != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return retval_dest;
	}

	ipc_call_t answer_router;
	aid_t req_router = async_data_read(exch, &srinfo->router,
	    sizeof(inet_addr_t), &answer_router);

	errno_t retval_router;
	async_wait_for(req_router, &retval_router);

	if (retval_router != EOK) {
		async_exchange_end(exch);
		async_forget(req);
		return retval_router;
	}

	ipc_call_t answer_name;
	char name_buf[LOC_NAME_MAXLEN + 1];
	aid_t req_name = async_data_read(exch, name_buf, LOC_NAME_MAXLEN,
	    &answer_name);

	async_exchange_end(exch);

	errno_t retval_name;
	async_wait_for(req_name, &retval_name);

	if (retval_name != EOK) {
		async_forget(req);
		return retval_name;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	if (retval != EOK)
		return retval;

	size_t act_size = ipc_get_arg2(&answer_name);
	assert(act_size <= LOC_NAME_MAXLEN);

	name_buf[act_size] = '\0';

	srinfo->name = str_dup(name_buf);

	return EOK;
}

errno_t inetcfg_sroute_get_id(const char *name, sysarg_t *sroute_id)
{
	async_exch_t *exch = async_exchange_begin(inetcfg_sess);

	ipc_call_t answer;
	aid_t req = async_send_0(exch, INETCFG_SROUTE_GET_ID, &answer);
	errno_t retval = async_data_write_start(exch, name, str_size(name));

	async_exchange_end(exch);

	if (retval != EOK) {
		async_forget(req);
		return retval;
	}

	async_wait_for(req, &retval);
	*sroute_id = ipc_get_arg1(&answer);

	return retval;
}

/** @}
 */
