/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup udp
 * @{
 */

/**
 * @file UDP client associations
 *
 * Ties UDP associations into the namespace of a client
 */

#include <errno.h>
#include <inet/endpoint.h>
#include <io/log.h>
#include <stdlib.h>

#include "cassoc.h"
#include "udp_type.h"

/** Add message to client receive queue.
 *
 * @param cassoc Client association
 * @param epp    Endpoint pair on which message was received
 * @param msg    Message
 *
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t udp_cassoc_queue_msg(udp_cassoc_t *cassoc, inet_ep2_t *epp,
    udp_msg_t *msg)
{
	udp_crcv_queue_entry_t *rqe;

	log_msg(LOG_DEFAULT, LVL_DEBUG, "udp_cassoc_queue_msg(%p, %p, %p)",
	    cassoc, epp, msg);

	rqe = calloc(1, sizeof(udp_crcv_queue_entry_t));
	if (rqe == NULL)
		return ENOMEM;

	link_initialize(&rqe->link);
	rqe->epp = *epp;
	rqe->msg = msg;
	rqe->cassoc = cassoc;

	list_append(&rqe->link, &cassoc->client->crcv_queue);
	return EOK;
}

/** Create client association.
 *
 * This effectively adds an association into a client's namespace.
 *
 * @param client  Client
 * @param assoc   Association
 * @param rcassoc Place to store pointer to new client association
 *
 * @return EOK on soccess, ENOMEM if out of memory
 */
errno_t udp_cassoc_create(udp_client_t *client, udp_assoc_t *assoc,
    udp_cassoc_t **rcassoc)
{
	udp_cassoc_t *cassoc;
	sysarg_t id;

	cassoc = calloc(1, sizeof(udp_cassoc_t));
	if (cassoc == NULL)
		return ENOMEM;

	/* Allocate new ID */
	id = 0;
	list_foreach (client->cassoc, lclient, udp_cassoc_t, cassoc) {
		if (cassoc->id >= id)
			id = cassoc->id + 1;
	}

	cassoc->id = id;
	cassoc->client = client;
	cassoc->assoc = assoc;

	list_append(&cassoc->lclient, &client->cassoc);
	*rcassoc = cassoc;
	return EOK;
}

/** Destroy client association.
 *
 * @param cassoc Client association
 */
void udp_cassoc_destroy(udp_cassoc_t *cassoc)
{
	list_remove(&cassoc->lclient);
	free(cassoc);
}

/** Get client association by ID.
 *
 * @param client  Client
 * @param id      Client association ID
 * @param rcassoc Place to store pointer to client association
 *
 * @return EOK on success, ENOENT if no client association with the given ID
 *         is found.
 */
errno_t udp_cassoc_get(udp_client_t *client, sysarg_t id,
    udp_cassoc_t **rcassoc)
{
	list_foreach (client->cassoc, lclient, udp_cassoc_t, cassoc) {
		if (cassoc->id == id) {
			*rcassoc = cassoc;
			return EOK;
		}
	}

	return ENOENT;
}

/**
 * @}
 */
