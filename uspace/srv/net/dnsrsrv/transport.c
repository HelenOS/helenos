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

/** @addtogroup dnsrsrv
 * @{
 */
/**
 * @file
 */

#include <adt/list.h>
#include <errno.h>
#include <str_error.h>
#include <fibril_synch.h>
#include <inet/addr.h>
#include <inet/endpoint.h>
#include <inet/udp.h>
#include <io/log.h>
#include <stdbool.h>
#include <stdlib.h>

#include "dns_msg.h"
#include "dns_type.h"
#include "transport.h"

#define RECV_BUF_SIZE 4096
#define DNS_SERVER_PORT 53

/** Request timeout (microseconds) */
#define REQ_TIMEOUT (5 * 1000 * 1000)

/** Maximum number of retries */
#define REQ_RETRY_MAX 3

inet_addr_t dns_server_addr;

typedef struct {
	link_t lreq;
	dns_message_t *req;
	dns_message_t *resp;

	bool done;
	fibril_condvar_t done_cv;
	fibril_mutex_t done_lock;

	errno_t status;
} trans_req_t;

static uint8_t recv_buf[RECV_BUF_SIZE];
static udp_t *transport_udp;
static udp_assoc_t *transport_assoc;

/** Outstanding requests */
static LIST_INITIALIZE(treq_list);
static FIBRIL_MUTEX_INITIALIZE(treq_lock);

static void transport_recv_msg(udp_assoc_t *, udp_rmsg_t *);
static void transport_recv_err(udp_assoc_t *, udp_rerr_t *);
static void transport_link_state(udp_assoc_t *, udp_link_state_t);

static udp_cb_t transport_cb = {
	.recv_msg = transport_recv_msg,
	.recv_err = transport_recv_err,
	.link_state = transport_link_state
};

errno_t transport_init(void)
{
	inet_ep2_t epp;
	errno_t rc;

	inet_ep2_init(&epp);

	rc = udp_create(&transport_udp);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	rc = udp_assoc_create(transport_udp, &epp, &transport_cb, NULL,
	    &transport_assoc);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	return EOK;
error:
	log_msg(LOG_DEFAULT, LVL_ERROR, "Failed initializing network.");
	udp_assoc_destroy(transport_assoc);
	udp_destroy(transport_udp);
	return rc;
}

void transport_fini(void)
{
	udp_assoc_destroy(transport_assoc);
	udp_destroy(transport_udp);
}

static trans_req_t *treq_create(dns_message_t *req)
{
	trans_req_t *treq;

	treq = calloc(1, sizeof(trans_req_t));
	if (treq == NULL)
		return NULL;

	treq->req = req;
	treq->resp = NULL;
	treq->done = false;
	fibril_condvar_initialize(&treq->done_cv);
	fibril_mutex_initialize(&treq->done_lock);

	fibril_mutex_lock(&treq_lock);
	list_append(&treq->lreq, &treq_list);
	fibril_mutex_unlock(&treq_lock);

	return treq;
}

static void treq_destroy(trans_req_t *treq)
{
	if (link_in_use(&treq->lreq))
		list_remove(&treq->lreq);
	free(treq);
}

static trans_req_t *treq_match_resp(dns_message_t *resp)
{
	assert(fibril_mutex_is_locked(&treq_lock));

	list_foreach(treq_list, lreq, trans_req_t, treq) {
		if (treq->req->id == resp->id) {
			/* Match */
			return treq;
		}
	}

	return NULL;
}

static void treq_complete(trans_req_t *treq, dns_message_t *resp)
{
	fibril_mutex_lock(&treq->done_lock);
	treq->done = true;
	treq->status = EOK;
	treq->resp = resp;
	fibril_mutex_unlock(&treq->done_lock);

	fibril_condvar_broadcast(&treq->done_cv);
}

errno_t dns_request(dns_message_t *req, dns_message_t **rresp)
{
	trans_req_t *treq = NULL;
	inet_ep_t ep;

	void *req_data;
	size_t req_size;
	log_msg(LOG_DEFAULT, LVL_DEBUG, "dns_request: Encode dns message");
	errno_t rc = dns_message_encode(req, &req_data, &req_size);
	if (rc != EOK)
		goto error;

	inet_ep_init(&ep);
	ep.addr = dns_server_addr;
	ep.port = DNS_SERVER_PORT;

	size_t ntry = 0;

	while (ntry < REQ_RETRY_MAX) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "dns_request: Send DNS message");
		rc = udp_assoc_send_msg(transport_assoc, &ep, req_data,
		    req_size);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Error sending message: %s", str_error(rc));
			goto error;
		}

		treq = treq_create(req);
		if (treq == NULL) {
			rc = ENOMEM;
			goto error;
		}

		fibril_mutex_lock(&treq->done_lock);
		while (treq->done != true) {
			rc = fibril_condvar_wait_timeout(&treq->done_cv, &treq->done_lock,
			    REQ_TIMEOUT);
			if (rc == ETIMEOUT) {
				++ntry;
				break;
			}
		}

		fibril_mutex_unlock(&treq->done_lock);

		if (rc != ETIMEOUT)
			break;
	}

	if (ntry >= REQ_RETRY_MAX) {
		rc = EIO;
		goto error;
	}

	if (treq->status != EOK) {
		rc = treq->status;
		goto error;
	}

	*rresp = treq->resp;
	treq_destroy(treq);
	free(req_data);
	return EOK;

error:
	if (treq != NULL)
		treq_destroy(treq);

	free(req_data);
	return rc;
}

static void transport_recv_msg(udp_assoc_t *assoc, udp_rmsg_t *rmsg)
{
	dns_message_t *resp = NULL;
	trans_req_t *treq;
	size_t size;
	inet_ep_t remote_ep;
	errno_t rc;

	size = udp_rmsg_size(rmsg);
	if (size > RECV_BUF_SIZE)
		size = RECV_BUF_SIZE; /* XXX */

	rc = udp_rmsg_read(rmsg, 0, recv_buf, size);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error reading message.");
		return;
	}

	udp_rmsg_remote_ep(rmsg, &remote_ep);
	/* XXX */

	rc = dns_message_decode(recv_buf, size, &resp);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "Error decoding message.");
		return;
	}

	assert(resp != NULL);

	fibril_mutex_lock(&treq_lock);
	treq = treq_match_resp(resp);
	if (treq == NULL) {
		fibril_mutex_unlock(&treq_lock);
		return;
	}

	list_remove(&treq->lreq);
	fibril_mutex_unlock(&treq_lock);

	treq_complete(treq, resp);
}

static void transport_recv_err(udp_assoc_t *assoc, udp_rerr_t *rerr)
{
	log_msg(LOG_DEFAULT, LVL_WARN, "Ignoring ICMP error");
}

static void transport_link_state(udp_assoc_t *assoc, udp_link_state_t ls)
{
	log_msg(LOG_DEFAULT, LVL_NOTE, "Link state change");
}

/** @}
 */
