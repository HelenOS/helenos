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

	/** Interface hash table */
	hash_table_t iface_hash_table;

	/** Broker session to the service */
	async_sess_t *broker_sess;
} hashed_service_t;

/** Interface hash table item. */
typedef struct {
	ht_link_t link;

	/** Interface ID */
	iface_t iface;

	/** Session to the service */
	async_sess_t *sess;
} hashed_iface_t;

static size_t service_key_hash(const void *key)
{
	const service_t *srv = key;
	return *srv;
}

static size_t service_hash(const ht_link_t *item)
{
	hashed_service_t *service =
	    hash_table_get_inst(item, hashed_service_t, link);

	return service->service;
}

static bool service_key_equal(const void *key, const ht_link_t *item)
{
	const service_t *srv = key;
	hashed_service_t *service =
	    hash_table_get_inst(item, hashed_service_t, link);

	return service->service == *srv;
}

static size_t iface_key_hash(const void *key)
{
	const iface_t *iface = key;
	return *iface;
}

static size_t iface_hash(const ht_link_t *item)
{
	hashed_iface_t *iface =
	    hash_table_get_inst(item, hashed_iface_t, link);

	return iface->iface;
}

static bool iface_key_equal(const void *key, const ht_link_t *item)
{
	const iface_t *kiface = key;
	hashed_iface_t *iface =
	    hash_table_get_inst(item, hashed_iface_t, link);

	return iface->iface == *kiface;
}

/** Operations for service hash table. */
static const hash_table_ops_t service_hash_table_ops = {
	.hash = service_hash,
	.key_hash = service_key_hash,
	.key_equal = service_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

/** Operations for interface hash table. */
static const hash_table_ops_t iface_hash_table_ops = {
	.hash = iface_hash,
	.key_hash = iface_key_hash,
	.key_equal = iface_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

/** Service hash table structure. */
static hash_table_t service_hash_table;

/** Pending connection structure. */
typedef struct {
	link_t link;
	service_t service;         /**< Service ID */
	iface_t iface;             /**< Interface ID */
	ipc_call_t call;           /**< Call waiting for the connection */
} pending_conn_t;

static list_t pending_conn;

errno_t ns_service_init(void)
{
	if (!hash_table_create(&service_hash_table, 0, 0,
	    &service_hash_table_ops)) {
		printf("%s: No memory available for services\n", NAME);
		return ENOMEM;
	}

	list_initialize(&pending_conn);

	return EOK;
}

static void ns_forward(async_sess_t *sess, ipc_call_t *call, iface_t iface)
{
	async_exch_t *exch = async_exchange_begin(sess);
	async_forward_1(call, exch, iface, ipc_get_arg3(call), IPC_FF_NONE);
	async_exchange_end(exch);
}

/** Process pending connection requests */
void ns_pending_conn_process(void)
{
loop:
	list_foreach(pending_conn, link, pending_conn_t, pending) {
		ht_link_t *link =
		    hash_table_find(&service_hash_table, &pending->service);
		if (!link)
			continue;

		hashed_service_t *hashed_service =
		    hash_table_get_inst(link, hashed_service_t, link);

		link = hash_table_find(&hashed_service->iface_hash_table,
		    &pending->iface);
		if (!link) {
			if (hashed_service->broker_sess != NULL) {
				ns_forward(hashed_service->broker_sess, &pending->call,
				    pending->iface);

				list_remove(&pending->link);
				free(pending);

				goto loop;
			}

			continue;
		}

		hashed_iface_t *hashed_iface =
		    hash_table_get_inst(link, hashed_iface_t, link);

		ns_forward(hashed_iface->sess, &pending->call, pending->iface);

		list_remove(&pending->link);
		free(pending);

		goto loop;
	}
}

/** Register interface to a service.
 *
 * @param service Service to which the interface belongs.
 * @param iface   Interface to be registered.
 *
 * @return Zero on success or a value from @ref errno.h.
 *
 */
static errno_t ns_iface_register(hashed_service_t *hashed_service, iface_t iface)
{
	ht_link_t *link = hash_table_find(&hashed_service->iface_hash_table,
	    &iface);
	if (link)
		return EEXIST;

	hashed_iface_t *hashed_iface =
	    (hashed_iface_t *) malloc(sizeof(hashed_iface_t));
	if (!hashed_iface)
		return ENOMEM;

	hashed_iface->iface = iface;
	hashed_iface->sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (hashed_iface->sess == NULL) {
		free(hashed_iface);
		return EIO;
	}

	hash_table_insert(&hashed_service->iface_hash_table,
	    &hashed_iface->link);
	return EOK;
}

/** Register broker to a service.
 *
 * @param service Service to which the broker belongs.
 *
 * @return Zero on success or a value from @ref errno.h.
 *
 */
static errno_t ns_broker_register(hashed_service_t *hashed_service)
{
	if (hashed_service->broker_sess != NULL)
		return EEXIST;

	hashed_service->broker_sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (hashed_service->broker_sess == NULL)
		return EIO;

	return EOK;
}

/** Register service.
 *
 * @param service Service to be registered.
 * @param iface   Interface to be registered.
 *
 * @return Zero on success or a value from @ref errno.h.
 *
 */
errno_t ns_service_register(service_t service, iface_t iface)
{
	ht_link_t *link = hash_table_find(&service_hash_table, &service);

	if (link) {
		hashed_service_t *hashed_service =
		    hash_table_get_inst(link, hashed_service_t, link);

		assert(hashed_service->service == service);

		return ns_iface_register(hashed_service, iface);
	}

	hashed_service_t *hashed_service =
	    (hashed_service_t *) malloc(sizeof(hashed_service_t));
	if (!hashed_service)
		return ENOMEM;

	if (!hash_table_create(&hashed_service->iface_hash_table, 0, 0,
	    &iface_hash_table_ops)) {
		free(hashed_service);
		return ENOMEM;
	}

	hashed_service->broker_sess = NULL;
	hashed_service->service = service;
	errno_t rc = ns_iface_register(hashed_service, iface);
	if (rc != EOK) {
		free(hashed_service);
		return rc;
	}

	hash_table_insert(&service_hash_table, &hashed_service->link);
	return EOK;
}

/** Register broker service.
 *
 * @param service Broker service to be registered.
 *
 * @return Zero on success or a value from @ref errno.h.
 *
 */
errno_t ns_service_register_broker(service_t service)
{
	ht_link_t *link = hash_table_find(&service_hash_table, &service);

	if (link) {
		hashed_service_t *hashed_service =
		    hash_table_get_inst(link, hashed_service_t, link);

		assert(hashed_service->service == service);

		return ns_broker_register(hashed_service);
	}

	hashed_service_t *hashed_service =
	    (hashed_service_t *) malloc(sizeof(hashed_service_t));
	if (!hashed_service)
		return ENOMEM;

	if (!hash_table_create(&hashed_service->iface_hash_table, 0, 0,
	    &iface_hash_table_ops)) {
		free(hashed_service);
		return ENOMEM;
	}

	hashed_service->broker_sess = NULL;
	hashed_service->service = service;
	errno_t rc = ns_broker_register(hashed_service);
	if (rc != EOK) {
		free(hashed_service);
		return rc;
	}

	hash_table_insert(&service_hash_table, &hashed_service->link);
	return EOK;
}

/** Add pending connection */
static errno_t ns_pending_conn_add(service_t service, iface_t iface,
    ipc_call_t *call)
{
	pending_conn_t *pending =
	    (pending_conn_t *) malloc(sizeof(pending_conn_t));
	if (!pending)
		return ENOMEM;

	link_initialize(&pending->link);
	pending->service = service;
	pending->iface = iface;
	pending->call = *call;

	list_append(&pending->link, &pending_conn);
	return EOK;
}

/** Connect client to service.
 *
 * @param service  Service to be connected to.
 * @param iface    Interface to be connected to.
 * @param call     Pointer to call structure.
 *
 * @return Zero on success or a value from @ref errno.h.
 *
 */
void ns_service_forward(service_t service, iface_t iface, ipc_call_t *call)
{
	sysarg_t flags = ipc_get_arg4(call);
	errno_t retval;

	ht_link_t *link = hash_table_find(&service_hash_table, &service);
	if (!link) {
		if (flags & IPC_FLAG_BLOCKING) {
			/* Blocking connection, add to pending list */
			errno_t rc = ns_pending_conn_add(service, iface, call);
			if (rc == EOK)
				return;

			retval = rc;
			goto out;
		}

		retval = ENOENT;
		goto out;
	}

	hashed_service_t *hashed_service =
	    hash_table_get_inst(link, hashed_service_t, link);

	link = hash_table_find(&hashed_service->iface_hash_table, &iface);
	if (!link) {
		if (hashed_service->broker_sess != NULL) {
			ns_forward(hashed_service->broker_sess, call, iface);
			return;
		}

		if (flags & IPC_FLAG_BLOCKING) {
			/* Blocking connection, add to pending list */
			errno_t rc = ns_pending_conn_add(service, iface, call);
			if (rc == EOK)
				return;

			retval = rc;
			goto out;
		}

		retval = ENOENT;
		goto out;
	}

	hashed_iface_t *hashed_iface =
	    hash_table_get_inst(link, hashed_iface_t, link);

	ns_forward(hashed_iface->sess, call, iface);
	return;

out:
	async_answer_0(call, retval);
}

/**
 * @}
 */
