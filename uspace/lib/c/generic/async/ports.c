/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

#define _LIBC_ASYNC_C_
#include <ipc/ipc.h>
#include <async.h>
#include "../private/async.h"
#undef _LIBC_ASYNC_C_

#include <ipc/irq.h>
#include <ipc/event.h>
#include <fibril.h>
#include <adt/hash_table.h>
#include <adt/hash.h>
#include <adt/list.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <barrier.h>
#include <stdbool.h>
#include <stdlib.h>
#include <mem.h>
#include <stdlib.h>
#include <macros.h>
#include <as.h>
#include <abi/mm/as.h>
#include "../private/libc.h"
#include "../private/fibril.h"

/** Interface data */
typedef struct {
	ht_link_t link;

	/** Interface ID */
	iface_t iface;

	/** Interface ports */
	hash_table_t port_hash_table;

	/** Next available port ID */
	port_id_t port_id_avail;
} interface_t;

/* Port data */
typedef struct {
	ht_link_t link;

	/** Port ID */
	port_id_t id;

	/** Port connection handler */
	async_port_handler_t handler;

	/** Client data */
	void *data;
} port_t;

/** Default fallback fibril function.
 *
 * This fallback fibril function gets called on incomming connections that do
 * not have a specific handler defined.
 *
 * @param call Data of the incoming call.
 * @param arg  Local argument
 *
 */
static void default_fallback_port_handler(ipc_call_t *call, void *arg)
{
	async_answer_0(call, ENOENT);
}

static async_port_handler_t fallback_port_handler =
    default_fallback_port_handler;
static void *fallback_port_data = NULL;

/** Futex guarding the interface hash table. */
static fibril_rmutex_t interface_mutex;
static hash_table_t interface_hash_table;

static size_t interface_key_hash(const void *key)
{
	const iface_t *iface = key;
	return *iface;
}

static size_t interface_hash(const ht_link_t *item)
{
	interface_t *interface = hash_table_get_inst(item, interface_t, link);
	return interface_key_hash(&interface->iface);
}

static bool interface_key_equal(const void *key, const ht_link_t *item)
{
	const iface_t *iface = key;
	interface_t *interface = hash_table_get_inst(item, interface_t, link);
	return *iface == interface->iface;
}

/** Operations for the port hash table. */
static const hash_table_ops_t interface_hash_table_ops = {
	.hash = interface_hash,
	.key_hash = interface_key_hash,
	.key_equal = interface_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

static size_t port_key_hash(const void *key)
{
	const port_id_t *port_id = key;
	return *port_id;
}

static size_t port_hash(const ht_link_t *item)
{
	port_t *port = hash_table_get_inst(item, port_t, link);
	return port_key_hash(&port->id);
}

static bool port_key_equal(const void *key, const ht_link_t *item)
{
	const port_id_t *port_id = key;
	port_t *port = hash_table_get_inst(item, port_t, link);
	return *port_id == port->id;
}

/** Operations for the port hash table. */
static const hash_table_ops_t port_hash_table_ops = {
	.hash = port_hash,
	.key_hash = port_key_hash,
	.key_equal = port_key_equal,
	.equal = NULL,
	.remove_callback = NULL
};

static interface_t *async_new_interface(iface_t iface)
{
	interface_t *interface =
	    (interface_t *) malloc(sizeof(interface_t));
	if (!interface)
		return NULL;

	bool ret = hash_table_create(&interface->port_hash_table, 0, 0,
	    &port_hash_table_ops);
	if (!ret) {
		free(interface);
		return NULL;
	}

	interface->iface = iface;
	interface->port_id_avail = 0;

	hash_table_insert(&interface_hash_table, &interface->link);

	return interface;
}

static port_t *async_new_port(interface_t *interface,
    async_port_handler_t handler, void *data)
{
	// TODO: Move the malloc out of critical section.
	port_t *port = (port_t *) malloc(sizeof(port_t));
	if (!port)
		return NULL;

	port_id_t id = interface->port_id_avail;
	interface->port_id_avail++;

	port->id = id;
	port->handler = handler;
	port->data = data;

	hash_table_insert(&interface->port_hash_table, &port->link);

	return port;
}

errno_t async_create_port_internal(iface_t iface, async_port_handler_t handler,
    void *data, port_id_t *port_id)
{
	interface_t *interface;

	fibril_rmutex_lock(&interface_mutex);

	ht_link_t *link = hash_table_find(&interface_hash_table, &iface);
	if (link)
		interface = hash_table_get_inst(link, interface_t, link);
	else
		interface = async_new_interface(iface);

	if (!interface) {
		fibril_rmutex_unlock(&interface_mutex);
		return ENOMEM;
	}

	port_t *port = async_new_port(interface, handler, data);
	if (!port) {
		fibril_rmutex_unlock(&interface_mutex);
		return ENOMEM;
	}

	*port_id = port->id;

	fibril_rmutex_unlock(&interface_mutex);

	return EOK;
}

errno_t async_create_port(iface_t iface, async_port_handler_t handler,
    void *data, port_id_t *port_id)
{
	if ((iface & IFACE_MOD_MASK) == IFACE_MOD_CALLBACK)
		return EINVAL;

	return async_create_port_internal(iface, handler, data, port_id);
}

void async_set_fallback_port_handler(async_port_handler_t handler, void *data)
{
	assert(handler != NULL);

	fallback_port_handler = handler;
	fallback_port_data = data;
}

static port_t *async_find_port(iface_t iface, port_id_t port_id)
{
	port_t *port = NULL;

	fibril_rmutex_lock(&interface_mutex);

	ht_link_t *link = hash_table_find(&interface_hash_table, &iface);
	if (link) {
		interface_t *interface =
		    hash_table_get_inst(link, interface_t, link);

		link = hash_table_find(&interface->port_hash_table, &port_id);
		if (link)
			port = hash_table_get_inst(link, port_t, link);
	}

	fibril_rmutex_unlock(&interface_mutex);

	return port;
}

async_port_handler_t async_get_port_handler(iface_t iface, port_id_t port_id,
    void **data)
{
	assert(data);

	async_port_handler_t handler = fallback_port_handler;
	*data = fallback_port_data;

	port_t *port = async_find_port(iface, port_id);
	if (port) {
		handler = port->handler;
		*data = port->data;
	}

	return handler;
}

/** Initialize the async framework.
 *
 */
void __async_ports_init(void)
{
	if (fibril_rmutex_initialize(&interface_mutex) != EOK)
		abort();

	if (!hash_table_create(&interface_hash_table, 0, 0,
	    &interface_hash_table_ops))
		abort();
}

void __async_ports_fini(void)
{
	fibril_rmutex_destroy(&interface_mutex);
}
