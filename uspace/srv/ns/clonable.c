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

#include <async.h>
#include <ipc/services.h>
#include <adt/list.h>
#include <stdbool.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <loader/loader.h>
#include "clonable.h"
#include "ns.h"

/** Request for connection to a clonable service. */
typedef struct {
	link_t link;
	service_t service;
	iface_t iface;
	ipc_call_t call;
} cs_req_t;

/** List of clonable-service connection requests. */
static list_t cs_req;

errno_t ns_clonable_init(void)
{
	list_initialize(&cs_req);
	return EOK;
}

/** Return true if @a service is clonable. */
bool ns_service_is_clonable(service_t service, iface_t iface)
{
	return (service == SERVICE_LOADER) && (iface == INTERFACE_LOADER);
}

/** Register clonable service.
 *
 * @param call Pointer to call structure.
 *
 */
void ns_clonable_register(ipc_call_t *call)
{
	link_t *req_link = list_first(&cs_req);
	if (req_link == NULL) {
		/* There was no pending connection request. */
		printf("%s: Unexpected clonable server.\n", NAME);
		async_answer_0(call, EBUSY);
		return;
	}

	cs_req_t *csr = list_get_instance(req_link, cs_req_t, link);
	list_remove(req_link);

	/* Currently we can only handle a single type of clonable service. */
	assert(ns_service_is_clonable(csr->service, csr->iface));

	async_answer_0(call, EOK);

	async_sess_t *sess = async_callback_receive(EXCHANGE_SERIALIZE);
	if (sess == NULL)
		async_answer_0(call, EIO);

	async_exch_t *exch = async_exchange_begin(sess);
	async_forward_1(&csr->call, exch, csr->iface,
	    ipc_get_arg3(&csr->call), IPC_FF_NONE);
	async_exchange_end(exch);

	free(csr);
	async_hangup(sess);
}

/** Connect client to clonable service.
 *
 * @param service Service to be connected to.
 * @param iface   Interface to be connected to.
 * @param call    Pointer to call structure.
 *
 * @return Zero on success or a value from @ref errno.h.
 *
 */
void ns_clonable_forward(service_t service, iface_t iface, ipc_call_t *call)
{
	assert(ns_service_is_clonable(service, iface));

	cs_req_t *csr = malloc(sizeof(cs_req_t));
	if (csr == NULL) {
		async_answer_0(call, ENOMEM);
		return;
	}

	/* Spawn a loader. */
	errno_t rc = loader_spawn("loader");

	if (rc != EOK) {
		free(csr);
		async_answer_0(call, rc);
		return;
	}

	link_initialize(&csr->link);
	csr->service = service;
	csr->iface = iface;
	csr->call = *call;

	/*
	 * We can forward the call only after the server we spawned connects
	 * to us. Meanwhile we might need to service more connection requests.
	 * Thus we store the call in a queue.
	 */
	list_append(&csr->link, &cs_req);
}

/**
 * @}
 */
