/*
 * SPDX-FileCopyrightText: 2013 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup http
 * @{
 */
/**
 * @file
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <macros.h>

#include <inet/endpoint.h>
#include <inet/host.h>
#include <inet/tcp.h>

#include <http/http.h>
#include <http/receive-buffer.h>

static errno_t http_receive(void *client_data, void *buf, size_t buf_size,
    size_t *nrecv)
{
	http_t *http = client_data;

	return tcp_conn_recv_wait(http->conn, buf, buf_size, nrecv);
}

http_t *http_create(const char *host, uint16_t port)
{
	http_t *http = malloc(sizeof(http_t));
	if (http == NULL)
		return NULL;

	http->host = str_dup(host);
	if (http->host == NULL) {
		free(http);
		return NULL;
	}
	http->port = port;

	http->buffer_size = 4096;
	errno_t rc = recv_buffer_init(&http->recv_buffer, http->buffer_size,
	    http_receive, http);
	if (rc != EOK) {
		free(http);
		return NULL;
	}

	return http;
}

errno_t http_connect(http_t *http)
{
	if (http->conn != NULL)
		return EBUSY;

	errno_t rc = inet_host_plookup_one(http->host, ip_any, &http->addr, NULL,
	    NULL);
	if (rc != EOK)
		return rc;

	inet_ep2_t epp;

	inet_ep2_init(&epp);
	epp.remote.addr = http->addr;
	epp.remote.port = http->port;

	rc = tcp_create(&http->tcp);
	if (rc != EOK)
		return rc;

	rc = tcp_conn_create(http->tcp, &epp, NULL, NULL, &http->conn);
	if (rc != EOK)
		return rc;

	rc = tcp_conn_wait_connected(http->conn);
	if (rc != EOK)
		return rc;

	return rc;
}

errno_t http_close(http_t *http)
{
	if (http->conn == NULL)
		return EINVAL;

	tcp_conn_destroy(http->conn);
	http->conn = NULL;
	tcp_destroy(http->tcp);
	http->tcp = NULL;

	return EOK;
}

void http_destroy(http_t *http)
{
	(void) http_close(http);
	recv_buffer_fini(&http->recv_buffer);
	free(http);
}

/** @}
 */
