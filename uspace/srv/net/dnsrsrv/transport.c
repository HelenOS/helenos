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

/** @addtogroup dnsres
 * @{
 */
/**
 * @file
 */

#include <adt/list.h>
#include <errno.h>
#include <fibril_synch.h>
#include <io/log.h>
#include <net/in.h>
#include <net/inet.h>
#include <net/socket.h>
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

	int status;
} trans_req_t;

static uint8_t recv_buf[RECV_BUF_SIZE];
static fid_t recv_fid;
static int transport_fd = -1;

/** Outstanding requests */
static LIST_INITIALIZE(treq_list);
static FIBRIL_MUTEX_INITIALIZE(treq_lock);

static int transport_recv_fibril(void *arg);

int transport_init(void)
{
	struct sockaddr_in laddr;
	int fd;
	fid_t fid;
	int rc;

	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(12345);
	laddr.sin_addr.s_addr = INADDR_ANY;

	fd = -1;

	fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (fd < 0) {
		rc = EIO;
		goto error;
	}

	rc = bind(fd, (struct sockaddr *)&laddr, sizeof(laddr));
	if (rc != EOK)
		goto error;

	transport_fd = fd;

	fid = fibril_create(transport_recv_fibril, NULL);
	if (fid == 0)
		goto error;

	fibril_add_ready(fid);
	recv_fid = fid;
	return EOK;
error:
	log_msg(LOG_DEFAULT, LVL_ERROR, "Failed initializing network socket.");
	if (fd >= 0)
		closesocket(fd);
	return rc;
}

void transport_fini(void)
{
	if (transport_fd >= 0)
		closesocket(transport_fd);
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

int dns_request(dns_message_t *req, dns_message_t **rresp)
{
	trans_req_t *treq = NULL;
	struct sockaddr *saddr = NULL;
	socklen_t saddrlen;
	
	void *req_data;
	size_t req_size;
	int rc = dns_message_encode(req, &req_data, &req_size);
	if (rc != EOK)
		goto error;
	
	rc = inet_addr_sockaddr(&dns_server_addr, DNS_SERVER_PORT,
	    &saddr, &saddrlen);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		goto error;
	}
	
	size_t ntry = 0;
	
	while (ntry < REQ_RETRY_MAX) {
		rc = sendto(transport_fd, req_data, req_size, 0,
		    saddr, saddrlen);
		if (rc != EOK)
			goto error;
		
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
	free(saddr);
	return EOK;
	
error:
	if (treq != NULL)
		treq_destroy(treq);
	
	free(req_data);
	free(saddr);
	return rc;
}

static int transport_recv_msg(dns_message_t **rresp)
{
	struct sockaddr_in src_addr;
	socklen_t src_addr_size;
	size_t recv_size;
	dns_message_t *resp;
	int rc;

	src_addr_size = sizeof(src_addr);
	rc = recvfrom(transport_fd, recv_buf, RECV_BUF_SIZE, 0,
	    (struct sockaddr *)&src_addr, &src_addr_size);
	if (rc < 0) {
		log_msg(LOG_DEFAULT, LVL_ERROR, "recvfrom returns error - %d", rc);
		goto error;
	}

	recv_size = (size_t)rc;

	rc = dns_message_decode(recv_buf, recv_size, &resp);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	*rresp = resp;
	return EOK;

error:
	return rc;
}

static int transport_recv_fibril(void *arg)
{
	dns_message_t *resp = NULL;
	trans_req_t *treq;
	int rc;

	while (true) {
		rc = transport_recv_msg(&resp);
		if (rc != EOK)
			continue;

		assert(resp != NULL);

		fibril_mutex_lock(&treq_lock);
		treq = treq_match_resp(resp);
		if (treq == NULL) {
			fibril_mutex_unlock(&treq_lock);
			continue;
		}

		list_remove(&treq->lreq);
		fibril_mutex_unlock(&treq_lock);

		treq_complete(treq, resp);
	}

	return 0;
}

/** @}
 */
