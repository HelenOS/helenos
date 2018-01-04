/*
 * Copyright (c) 2009 Martin Decky
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

/** @addtogroup ns
 * @{
 */

#include <adt/hash_table.h>
#include <assert.h>
#include <async.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "service.h"
#include "ns.h"

/** Service hash table item. */
typedef struct {
	ht_link_t link;
	
	/** Service ID */
	service_t service;
	
	/** Session to the service */
	async_sess_t *sess;
} hashed_service_t;

static size_t service_key_hash(void *key)
{
	return *(service_t *) key;
}

static size_t service_hash(const ht_link_t *item)
{
	hashed_service_t *service =
	    hash_table_get_inst(item, hashed_service_t, link);
	
	return service->service;
}

static bool service_key_equal(void *key, const ht_link_t *item)
{
	hashed_service_t *service =
	    hash_table_get_inst(item, hashed_service_t, link);
	
	return service->service == *(service_t *) key;
}

/** Operations for service hash table. */
static hash_table_ops_t service_hash_table_ops = {
	.hash = service_hash,
	.key_hash = service_key_hash,
	.key_equal = service_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

/** Service hash table structure. */
static hash_table_t service_hash_table;

/** Pending connection structure. */
typedef struct {
	link_t link;
	service_t service;    /**< Service ID */
	iface_t iface;        /**< Interface ID */
	ipc_callid_t callid;  /**< Call ID waiting for the connection */
	sysarg_t arg3;        /**< Third argument */
} pending_conn_t;

static list_t pending_conn;

errno_t service_init(void)
{
	if (!hash_table_create(&service_hash_table, 0, 0,
	    &service_hash_table_ops)) {
		printf("%s: No memory available for services\n", NAME);
		return ENOMEM;
	}
	
	list_initialize(&pending_conn);
	
	return EOK;
}

/** Process pending connection requests */
void process_pending_conn(void)
{
loop:
	list_foreach(pending_conn, link, pending_conn_t, pending) {
		ht_link_t *link = hash_table_find(&service_hash_table, &pending->service);
		if (!link)
			continue;
		
		hashed_service_t *hashed_service = hash_table_get_inst(link, hashed_service_t, link);
		async_exch_t *exch = async_exchange_begin(hashed_service->sess);
		async_forward_fast(pending->callid, exch, pending->iface,
		    pending->arg3, 0, IPC_FF_NONE);
		async_exchange_end(exch);
		
		list_remove(&pending->link);
		free(pending);
		
		goto loop;
	}
}

/** Register service.
 *
 * @param service Service to be registered.
 * @param phone   Phone to be used for connections to the service.
 * @param call    Pointer to call structure.
 *
 * @return Zero on success or a value from @ref errno.h.
 *
 */
errno_t register_service(service_t service, sysarg_t phone, ipc_call_t *call)
{
	if (hash_table_find(&service_hash_table, &service))
		return EEXIST;
	
	hashed_service_t *hashed_service =
	    (hashed_service_t *) malloc(sizeof(hashed_service_t));
	if (!hashed_service)
		return ENOMEM;
	
	hashed_service->service = service;
	hashed_service->sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (hashed_service->sess == NULL)
		return EIO;
	
	hash_table_insert(&service_hash_table, &hashed_service->link);
	return EOK;
}

/** Connect client to service.
 *
 * @param service Service to be connected to.
 * @param iface   Interface to be connected to.
 * @param call    Pointer to call structure.
 * @param callid  Call ID of the request.
 *
 * @return Zero on success or a value from @ref errno.h.
 *
 */
void connect_to_service(service_t service, iface_t iface, ipc_call_t *call,
    ipc_callid_t callid)
{
	sysarg_t arg3 = IPC_GET_ARG3(*call);
	sysarg_t flags = IPC_GET_ARG4(*call);
	errno_t retval;
	
	ht_link_t *link = hash_table_find(&service_hash_table, &service);
	if (!link) {
		if (flags & IPC_FLAG_BLOCKING) {
			/* Blocking connection, add to pending list */
			pending_conn_t *pending =
			    (pending_conn_t *) malloc(sizeof(pending_conn_t));
			if (!pending) {
				retval = ENOMEM;
				goto out;
			}
			
			link_initialize(&pending->link);
			pending->service = service;
			pending->iface = iface;
			pending->callid = callid;
			pending->arg3 = arg3;
			
			list_append(&pending->link, &pending_conn);
			return;
		}
		
		retval = ENOENT;
		goto out;
	}
	
	hashed_service_t *hashed_service = hash_table_get_inst(link, hashed_service_t, link);
	async_exch_t *exch = async_exchange_begin(hashed_service->sess);
	async_forward_fast(callid, exch, iface, arg3, 0, IPC_FF_NONE);
	async_exchange_end(exch);
	return;
	
out:
	async_answer_0(callid, retval);
}

/**
 * @}
 */
