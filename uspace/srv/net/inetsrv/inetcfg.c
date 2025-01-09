/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup inet
 * @{
 */
/**
 * @file
 * @brief
 */

#include <async.h>
#include <errno.h>
#include <macros.h>
#include <io/log.h>
#include <ipc/inet.h>
#include <loc.h>
#include <stdlib.h>
#include <str.h>
#include <stddef.h>
#include <types/inetcfg.h>

#include "addrobj.h"
#include "inetsrv.h"
#include "inet_link.h"
#include "inetcfg.h"
#include "sroute.h"

static errno_t inetcfg_addr_create_static(char *name, inet_naddr_t *naddr,
    sysarg_t link_id, sysarg_t *addr_id)
{
	inet_link_t *ilink;
	inet_addrobj_t *addr;
	inet_addr_t iaddr;
	errno_t rc;

	ilink = inet_link_get_by_id(link_id);
	if (ilink == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Link %lu not found.",
		    (unsigned long) link_id);
		return ENOENT;
	}

	addr = inet_addrobj_new();
	if (addr == NULL) {
		*addr_id = 0;
		return ENOMEM;
	}

	addr->naddr = *naddr;
	addr->ilink = ilink;
	addr->name = str_dup(name);
	rc = inet_addrobj_add(addr);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Duplicate address name '%s'.", addr->name);
		inet_addrobj_delete(addr);
		return rc;
	}

	inet_naddr_addr(&addr->naddr, &iaddr);
	rc = iplink_addr_add(ilink->iplink, &iaddr);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Failed setting IP address on internet link.");
		inet_addrobj_remove(addr);
		inet_addrobj_delete(addr);
		return rc;
	}

	rc = inet_cfg_sync(cfg);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error saving configuration.");
		return rc;
	}

	return EOK;
}

static errno_t inetcfg_addr_delete(sysarg_t addr_id)
{
	inet_addrobj_t *addr;
	inet_link_cfg_info_t info;
	unsigned acnt;
	inet_link_t *ilink;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "inetcfg_addr_delete()");

	addr = inet_addrobj_get_by_id(addr_id);
	if (addr == NULL)
		return ENOENT;

	info.svc_id = addr->ilink->svc_id;
	info.svc_name = str_dup(addr->ilink->svc_name);
	if (info.svc_name == NULL)
		return ENOMEM;

	inet_addrobj_remove(addr);
	inet_addrobj_delete(addr);

	log_msg(LOG_DEFAULT, LVL_NOTE, "inetcfg_addr_delete(): sync");

	rc = inet_cfg_sync(cfg);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error saving configuration.");
		free(info.svc_name);
		return rc;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "inetcfg_addr_delete(): get link by ID");

	ilink = inet_link_get_by_id(info.svc_id);
	if (ilink == NULL) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error finding link.");
		return ENOENT;
	}

	log_msg(LOG_DEFAULT, LVL_NOTE, "inetcfg_addr_delete(): check addrobj count");

	/* If there are no configured addresses left, autoconfigure link */
	acnt = inet_addrobj_cnt_by_link(ilink);
	log_msg(LOG_DEFAULT, LVL_NOTE, "inetcfg_addr_delete(): acnt=%u", acnt);
	if (acnt == 0)
		inet_link_autoconf_link(&info);

	log_msg(LOG_DEFAULT, LVL_NOTE, "inetcfg_addr_delete(): DONE");
	return EOK;
}

static errno_t inetcfg_addr_get(sysarg_t addr_id, inet_addr_info_t *ainfo)
{
	inet_addrobj_t *addr;

	addr = inet_addrobj_get_by_id(addr_id);
	if (addr == NULL)
		return ENOENT;

	ainfo->naddr = addr->naddr;
	ainfo->ilink = addr->ilink->svc_id;
	ainfo->name = str_dup(addr->name);

	return EOK;
}

static errno_t inetcfg_addr_get_id(char *name, sysarg_t link_id, sysarg_t *addr_id)
{
	inet_link_t *ilink;
	inet_addrobj_t *addr;

	ilink = inet_link_get_by_id(link_id);
	if (ilink == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Link %zu not found.", (size_t) link_id);
		return ENOENT;
	}

	addr = inet_addrobj_find_by_name(name, ilink);
	if (addr == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Address '%s' not found.", name);
		return ENOENT;
	}

	*addr_id = addr->id;
	return EOK;
}

static errno_t inetcfg_get_addr_list(sysarg_t **addrs, size_t *count)
{
	return inet_addrobj_get_id_list(addrs, count);
}

static errno_t inetcfg_get_link_list(sysarg_t **addrs, size_t *count)
{
	return inet_link_get_id_list(addrs, count);
}

static errno_t inetcfg_get_sroute_list(sysarg_t **sroutes, size_t *count)
{
	return inet_sroute_get_id_list(sroutes, count);
}

static errno_t inetcfg_link_add(sysarg_t link_id)
{
	return inet_link_open(link_id);
}

static errno_t inetcfg_link_get(sysarg_t link_id, inet_link_info_t *linfo)
{
	inet_link_t *ilink;

	ilink = inet_link_get_by_id(link_id);
	if (ilink == NULL) {
		return ENOENT;
	}

	linfo->name = str_dup(ilink->svc_name);
	linfo->def_mtu = ilink->def_mtu;
	if (ilink->mac_valid) {
		linfo->mac_addr = ilink->mac;
	} else {
		memset(&linfo->mac_addr, 0, sizeof(linfo->mac_addr));
	}

	return EOK;
}

static errno_t inetcfg_link_remove(sysarg_t link_id)
{
	return ENOTSUP;
}

static errno_t inetcfg_sroute_create(char *name, inet_naddr_t *dest,
    inet_addr_t *router, sysarg_t *sroute_id)
{
	errno_t rc;
	inet_sroute_t *sroute;

	sroute = inet_sroute_new();
	if (sroute == NULL) {
		*sroute_id = 0;
		return ENOMEM;
	}

	sroute->dest = *dest;
	sroute->router = *router;
	sroute->name = str_dup(name);
	inet_sroute_add(sroute);

	*sroute_id = sroute->id;

	rc = inet_cfg_sync(cfg);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error saving configuration.");
		return rc;
	}

	return EOK;
}

static errno_t inetcfg_sroute_delete(sysarg_t sroute_id)
{
	errno_t rc;
	inet_sroute_t *sroute;

	sroute = inet_sroute_get_by_id(sroute_id);
	if (sroute == NULL)
		return ENOENT;

	inet_sroute_remove(sroute);
	inet_sroute_delete(sroute);

	rc = inet_cfg_sync(cfg);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error saving configuration.");
		return rc;
	}

	return EOK;
}

static errno_t inetcfg_sroute_get(sysarg_t sroute_id, inet_sroute_info_t *srinfo)
{
	inet_sroute_t *sroute;

	sroute = inet_sroute_get_by_id(sroute_id);
	if (sroute == NULL)
		return ENOENT;

	srinfo->dest = sroute->dest;
	srinfo->router = sroute->router;
	srinfo->name = str_dup(sroute->name);

	return EOK;
}

static errno_t inetcfg_sroute_get_id(char *name, sysarg_t *sroute_id)
{
	inet_sroute_t *sroute;

	sroute = inet_sroute_find_by_name(name);
	if (sroute == NULL) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Static route '%s' not found.", name);
		return ENOENT;
	}

	*sroute_id = sroute->id;
	return EOK;
}

static void inetcfg_addr_create_static_srv(ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_addr_create_static_srv()");

	sysarg_t link_id = ipc_get_arg1(icall);

	ipc_call_t call;
	size_t size;
	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	if (size != sizeof(inet_naddr_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	inet_naddr_t naddr;
	errno_t rc = async_data_write_finalize(&call, &naddr, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	char *name;
	rc = async_data_write_accept((void **) &name, true, 0, LOC_NAME_MAXLEN,
	    0, NULL);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	sysarg_t addr_id = 0;
	rc = inetcfg_addr_create_static(name, &naddr, link_id, &addr_id);
	free(name);
	async_answer_1(icall, rc, addr_id);
}

static void inetcfg_addr_delete_srv(ipc_call_t *call)
{
	sysarg_t addr_id;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_addr_delete_srv()");

	addr_id = ipc_get_arg1(call);

	rc = inetcfg_addr_delete(addr_id);
	async_answer_0(call, rc);
}

static void inetcfg_addr_get_srv(ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_addr_get_srv()");

	sysarg_t addr_id = ipc_get_arg1(icall);

	inet_addr_info_t ainfo;

	inet_naddr_any(&ainfo.naddr);
	ainfo.ilink = 0;
	ainfo.name = NULL;

	errno_t rc = inetcfg_addr_get(addr_id, &ainfo);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	ipc_call_t call;
	size_t size;
	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(inet_naddr_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_read_finalize(&call, &ainfo.naddr, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	rc = async_data_read_finalize(&call, ainfo.name,
	    min(size, str_size(ainfo.name)));
	free(ainfo.name);

	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	async_answer_1(icall, rc, ainfo.ilink);
}

static void inetcfg_addr_get_id_srv(ipc_call_t *call)
{
	char *name;
	sysarg_t link_id;
	sysarg_t addr_id;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_addr_get_id_srv()");

	link_id = ipc_get_arg1(call);

	rc = async_data_write_accept((void **) &name, true, 0, LOC_NAME_MAXLEN,
	    0, NULL);
	if (rc != EOK) {
		async_answer_0(call, rc);
		return;
	}

	addr_id = 0;
	rc = inetcfg_addr_get_id(name, link_id, &addr_id);
	free(name);
	async_answer_1(call, rc, addr_id);
}

static void inetcfg_get_addr_list_srv(ipc_call_t *call)
{
	ipc_call_t rcall;
	size_t count;
	size_t max_size;
	size_t act_size;
	size_t size;
	sysarg_t *id_buf;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_get_addr_list_srv()");

	if (!async_data_read_receive(&rcall, &max_size)) {
		async_answer_0(&rcall, EREFUSED);
		async_answer_0(call, EREFUSED);
		return;
	}

	rc = inetcfg_get_addr_list(&id_buf, &count);
	if (rc != EOK) {
		async_answer_0(&rcall, rc);
		async_answer_0(call, rc);
		return;
	}

	act_size = count * sizeof(sysarg_t);
	size = min(act_size, max_size);

	errno_t retval = async_data_read_finalize(&rcall, id_buf, size);
	free(id_buf);

	async_answer_1(call, retval, act_size);
}

static void inetcfg_get_link_list_srv(ipc_call_t *call)
{
	ipc_call_t rcall;
	size_t count;
	size_t max_size;
	size_t act_size;
	size_t size;
	sysarg_t *id_buf;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_get_addr_list_srv()");

	if (!async_data_read_receive(&rcall, &max_size)) {
		async_answer_0(&rcall, EREFUSED);
		async_answer_0(call, EREFUSED);
		return;
	}

	rc = inetcfg_get_link_list(&id_buf, &count);
	if (rc != EOK) {
		async_answer_0(&rcall, rc);
		async_answer_0(call, rc);
		return;
	}

	act_size = count * sizeof(sysarg_t);
	size = min(act_size, max_size);

	errno_t retval = async_data_read_finalize(&rcall, id_buf, size);
	free(id_buf);

	async_answer_1(call, retval, act_size);
}

static void inetcfg_get_sroute_list_srv(ipc_call_t *call)
{
	ipc_call_t rcall;
	size_t count;
	size_t max_size;
	size_t act_size;
	size_t size;
	sysarg_t *id_buf;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_get_sroute_list_srv()");

	if (!async_data_read_receive(&rcall, &max_size)) {
		async_answer_0(&rcall, EREFUSED);
		async_answer_0(call, EREFUSED);
		return;
	}

	rc = inetcfg_get_sroute_list(&id_buf, &count);
	if (rc != EOK) {
		async_answer_0(&rcall, rc);
		async_answer_0(call, rc);
		return;
	}

	act_size = count * sizeof(sysarg_t);
	size = min(act_size, max_size);

	errno_t retval = async_data_read_finalize(&rcall, id_buf, size);
	free(id_buf);

	async_answer_1(call, retval, act_size);
}

static void inetcfg_link_add_srv(ipc_call_t *call)
{
	sysarg_t link_id;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_link_add_srv()");

	link_id = ipc_get_arg1(call);

	rc = inetcfg_link_add(link_id);
	async_answer_0(call, rc);
}

static void inetcfg_link_get_srv(ipc_call_t *call)
{
	ipc_call_t name;
	ipc_call_t laddr;
	size_t name_max_size;
	size_t laddr_max_size;

	sysarg_t link_id;
	inet_link_info_t linfo;
	errno_t rc;

	link_id = ipc_get_arg1(call);
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_link_get_srv()");

	linfo.name = NULL;

	if (!async_data_read_receive(&name, &name_max_size)) {
		async_answer_0(&name, EREFUSED);
		async_answer_0(call, EREFUSED);
		return;
	}

	if (!async_data_read_receive(&laddr, &laddr_max_size)) {
		async_answer_0(&laddr, EREFUSED);
		async_answer_0(&name, EREFUSED);
		async_answer_0(call, EREFUSED);
		return;
	}

	rc = inetcfg_link_get(link_id, &linfo);
	if (rc != EOK) {
		async_answer_0(&laddr, rc);
		async_answer_0(&name, rc);
		async_answer_0(call, rc);
		return;
	}

	errno_t retval = async_data_read_finalize(&name, linfo.name,
	    min(name_max_size, str_size(linfo.name)));
	if (retval != EOK) {
		free(linfo.name);
		async_answer_0(&laddr, retval);
		async_answer_0(call, retval);
		return;
	}

	retval = async_data_read_finalize(&laddr, &linfo.mac_addr,
	    min(laddr_max_size, sizeof(linfo.mac_addr)));

	free(linfo.name);

	async_answer_1(call, retval, linfo.def_mtu);
}

static void inetcfg_link_remove_srv(ipc_call_t *call)
{
	sysarg_t link_id;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_link_remove_srv()");

	link_id = ipc_get_arg1(call);

	rc = inetcfg_link_remove(link_id);
	async_answer_0(call, rc);
}

static void inetcfg_sroute_create_srv(ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_sroute_create_srv()");

	ipc_call_t call;
	size_t size;
	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	if (size != sizeof(inet_naddr_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	inet_naddr_t dest;
	errno_t rc = async_data_write_finalize(&call, &dest, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	if (!async_data_write_receive(&call, &size)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	if (size != sizeof(inet_addr_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	inet_addr_t router;
	rc = async_data_write_finalize(&call, &router, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	char *name;
	rc = async_data_write_accept((void **) &name, true, 0, LOC_NAME_MAXLEN,
	    0, NULL);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	sysarg_t sroute_id = 0;
	rc = inetcfg_sroute_create(name, &dest, &router, &sroute_id);
	free(name);
	async_answer_1(icall, rc, sroute_id);
}

static void inetcfg_sroute_delete_srv(ipc_call_t *call)
{
	sysarg_t sroute_id;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_sroute_delete_srv()");

	sroute_id = ipc_get_arg1(call);

	rc = inetcfg_sroute_delete(sroute_id);
	async_answer_0(call, rc);
}

static void inetcfg_sroute_get_srv(ipc_call_t *icall)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_sroute_get_srv()");

	sysarg_t sroute_id = ipc_get_arg1(icall);

	inet_sroute_info_t srinfo;

	inet_naddr_any(&srinfo.dest);
	inet_addr_any(&srinfo.router);
	srinfo.name = NULL;

	errno_t rc = inetcfg_sroute_get(sroute_id, &srinfo);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	ipc_call_t call;
	size_t size;
	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(inet_naddr_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_read_finalize(&call, &srinfo.dest, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	if (size != sizeof(inet_addr_t)) {
		async_answer_0(&call, EINVAL);
		async_answer_0(icall, EINVAL);
		return;
	}

	rc = async_data_read_finalize(&call, &srinfo.router, size);
	if (rc != EOK) {
		async_answer_0(&call, rc);
		async_answer_0(icall, rc);
		return;
	}

	if (!async_data_read_receive(&call, &size)) {
		async_answer_0(&call, EREFUSED);
		async_answer_0(icall, EREFUSED);
		return;
	}

	rc = async_data_read_finalize(&call, srinfo.name,
	    min(size, str_size(srinfo.name)));
	free(srinfo.name);

	async_answer_0(icall, rc);
}

static void inetcfg_sroute_get_id_srv(ipc_call_t *call)
{
	char *name;
	sysarg_t sroute_id;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inetcfg_sroute_get_id_srv()");

	rc = async_data_write_accept((void **) &name, true, 0, LOC_NAME_MAXLEN,
	    0, NULL);
	if (rc != EOK) {
		async_answer_0(call, rc);
		return;
	}

	sroute_id = 0;
	rc = inetcfg_sroute_get_id(name, &sroute_id);
	free(name);
	async_answer_1(call, rc, sroute_id);
}

void inet_cfg_conn(ipc_call_t *icall, void *arg)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_cfg_conn()");

	/* Accept the connection */
	async_accept_0(icall);

	while (true) {
		ipc_call_t call;
		async_get_call(&call);
		sysarg_t method = ipc_get_imethod(&call);

		log_msg(LOG_DEFAULT, LVL_DEBUG, "method %d", (int)method);
		if (!method) {
			/* The other side has hung up */
			async_answer_0(&call, EOK);
			return;
		}

		switch (method) {
		case INETCFG_ADDR_CREATE_STATIC:
			inetcfg_addr_create_static_srv(&call);
			break;
		case INETCFG_ADDR_DELETE:
			inetcfg_addr_delete_srv(&call);
			break;
		case INETCFG_ADDR_GET:
			inetcfg_addr_get_srv(&call);
			break;
		case INETCFG_ADDR_GET_ID:
			inetcfg_addr_get_id_srv(&call);
			break;
		case INETCFG_GET_ADDR_LIST:
			inetcfg_get_addr_list_srv(&call);
			break;
		case INETCFG_GET_LINK_LIST:
			inetcfg_get_link_list_srv(&call);
			break;
		case INETCFG_GET_SROUTE_LIST:
			inetcfg_get_sroute_list_srv(&call);
			break;
		case INETCFG_LINK_ADD:
			inetcfg_link_add_srv(&call);
			break;
		case INETCFG_LINK_GET:
			inetcfg_link_get_srv(&call);
			break;
		case INETCFG_LINK_REMOVE:
			inetcfg_link_remove_srv(&call);
			break;
		case INETCFG_SROUTE_CREATE:
			inetcfg_sroute_create_srv(&call);
			break;
		case INETCFG_SROUTE_DELETE:
			inetcfg_sroute_delete_srv(&call);
			break;
		case INETCFG_SROUTE_GET:
			inetcfg_sroute_get_srv(&call);
			break;
		case INETCFG_SROUTE_GET_ID:
			inetcfg_sroute_get_id_srv(&call);
			break;
		default:
			async_answer_0(&call, EINVAL);
		}
	}
}

static errno_t inet_cfg_load(const char *cfg_path)
{
	sif_doc_t *doc = NULL;
	sif_node_t *rnode;
	sif_node_t *naddrs;
	sif_node_t *nroutes;
	const char *ntype;
	errno_t rc;

	rc = sif_load(cfg_path, &doc);
	if (rc != EOK)
		goto error;

	rnode = sif_get_root(doc);
	naddrs = sif_node_first_child(rnode);
	ntype = sif_node_get_type(naddrs);
	if (str_cmp(ntype, "addresses") != 0) {
		rc = EIO;
		goto error;
	}

	rc = inet_addrobjs_load(naddrs);
	if (rc != EOK)
		goto error;

	nroutes = sif_node_next_child(naddrs);
	ntype = sif_node_get_type(nroutes);
	if (str_cmp(ntype, "static-routes") != 0) {
		rc = EIO;
		goto error;
	}

	rc = inet_sroutes_load(nroutes);
	if (rc != EOK)
		goto error;

	sif_delete(doc);
	return EOK;
error:
	if (doc != NULL)
		sif_delete(doc);
	return rc;

}

static errno_t inet_cfg_save(const char *cfg_path)
{
	sif_doc_t *doc = NULL;
	sif_node_t *rnode;
	sif_node_t *nsroutes;
	sif_node_t *naddrobjs;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_NOTE, "inet_cfg_save(%s)", cfg_path);

	rc = sif_new(&doc);
	if (rc != EOK)
		goto error;

	rnode = sif_get_root(doc);

	/* Address objects */

	rc = sif_node_append_child(rnode, "addresses", &naddrobjs);
	if (rc != EOK)
		goto error;

	rc = inet_addrobjs_save(naddrobjs);
	if (rc != EOK)
		goto error;

	/* Static routes */

	rc = sif_node_append_child(rnode, "static-routes", &nsroutes);
	if (rc != EOK)
		goto error;

	rc = inet_sroutes_save(nsroutes);
	if (rc != EOK)
		goto error;

	/* Save */

	rc = sif_save(doc, cfg_path);
	if (rc != EOK)
		goto error;

	sif_delete(doc);
	return EOK;
error:
	if (doc != NULL)
		sif_delete(doc);
	return rc;
}

/** Open internet server configuration.
 *
 * @param cfg_path Configuration file path
 * @param rcfg Place to store pointer to configuration object
 * @return EOK on success or an error code
 */
errno_t inet_cfg_open(const char *cfg_path, inet_cfg_t **rcfg)
{
	inet_cfg_t *cfg;
	errno_t rc;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "inet_cfg_open(%s)", cfg_path);

	rc = inet_cfg_load(cfg_path);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_WARN, "inet_cfg_open(%s) :"
		    "could not load configuration.", cfg_path);
	}

	cfg = calloc(1, sizeof(inet_cfg_t));
	if (cfg == NULL)
		return ENOMEM;

	cfg->cfg_path = str_dup(cfg_path);
	if (cfg->cfg_path == NULL) {
		free(cfg);
		return ENOMEM;
	}

	*rcfg = cfg;
	return EOK;
}

errno_t inet_cfg_sync(inet_cfg_t *cfg)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "inet_cfg_sync(cfg=%p)", cfg);
	return inet_cfg_save(cfg->cfg_path);
}

void inet_cfg_close(inet_cfg_t *cfg)
{
	free(cfg);
}

/** @}
 */
