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

/** @addtogroup libc
 * @{
 */
/** @file UDP API
 */

#include <errno.h>
#include <inet/endpoint.h>
#include <inet/tcp.h>

int tcp_create(tcp_t **rtcp)
{
	return 0;
}

void tcp_destroy(tcp_t *tcp)
{
}

int tcp_conn_create(tcp_t *tcp, inet_ep2_t *epp, tcp_cb_t *cb, void *arg,
    tcp_conn_t **rconn)
{
	return 0;
}

void tcp_conn_destroy(tcp_conn_t *conn)
{
}

void *tcp_conn_userptr(tcp_conn_t *conn)
{
	return NULL;
}

int tcp_listener_create(tcp_t *tcp, inet_ep_t *ep, tcp_listen_cb_t *lcb,
    void *larg, tcp_cb_t *cb, void *arg, tcp_listener_t **rlst)
{
	return 0;
}

void tcp_listener_destroy(tcp_listener_t *lst)
{
}

void *tcp_listener_userptr(tcp_listener_t *lst)
{
	return NULL;
}


int tcp_conn_wait_connected(tcp_conn_t *conn)
{
	return 0;
}

int tcp_conn_send(tcp_conn_t *conn, const void *data, size_t bytes)
{
	return 0;
}

int tcp_conn_send_fin(tcp_conn_t *conn)
{
	return 0;
}

int tcp_conn_push(tcp_conn_t *conn)
{
	return 0;
}

void tcp_conn_reset(tcp_conn_t *conn)
{
}


int tcp_conn_recv(tcp_conn_t *conn, void *buf, size_t bsize, size_t *nrecv)
{
	return 0;
}

int tcp_conn_recv_wait(tcp_conn_t *conn, void *buf, size_t bsize, size_t *nrecv)
{
	return 0;
}


/** @}
 */
