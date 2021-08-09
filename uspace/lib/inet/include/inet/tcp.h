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

/** @addtogroup libinet
 * @{
 */
/** @file
 */

#ifndef LIBINET_INET_TCP_H
#define LIBINET_INET_TCP_H

#include <fibril_synch.h>
#include <inet/addr.h>
#include <inet/endpoint.h>
#include <inet/inet.h>

/** TCP connection */
typedef struct {
	fibril_mutex_t lock;
	fibril_condvar_t cv;
	struct tcp *tcp;
	link_t ltcp;
	sysarg_t id;
	struct tcp_cb *cb;
	void *cb_arg;
	/** Some received data available in TCP server */
	bool data_avail;
	bool connected;
	bool conn_failed;
	bool conn_reset;
} tcp_conn_t;

/** TCP connection listener */
typedef struct {
	struct tcp *tcp;
	link_t ltcp;
	sysarg_t id;
	struct tcp_listen_cb *lcb;
	void *lcb_arg;
	struct tcp_cb *cb;
	void *cb_arg;
} tcp_listener_t;

/** TCP connection callbacks */
typedef struct tcp_cb {
	void (*connected)(tcp_conn_t *);
	void (*conn_failed)(tcp_conn_t *);
	void (*conn_reset)(tcp_conn_t *);
	void (*data_avail)(tcp_conn_t *);
	void (*urg_data)(tcp_conn_t *);
} tcp_cb_t;

/** TCP listener callbacks */
typedef struct tcp_listen_cb {
	void (*new_conn)(tcp_listener_t *, tcp_conn_t *);
} tcp_listen_cb_t;

/** TCP service */
typedef struct tcp {
	/** TCP session */
	async_sess_t *sess;
	/** List of connections */
	list_t conn; /* of tcp_conn_t */
	/** List of listeners */
	list_t listener; /* of tcp_listener_t */
	/** TCP service lock */
	fibril_mutex_t lock;
	/** For waiting on cb_done */
	fibril_condvar_t cv;
	/** Set to @a true when callback connection handler has terminated */
	bool cb_done;
} tcp_t;

extern errno_t tcp_create(tcp_t **);
extern void tcp_destroy(tcp_t *);
extern errno_t tcp_conn_create(tcp_t *, inet_ep2_t *, tcp_cb_t *, void *,
    tcp_conn_t **);
extern void tcp_conn_destroy(tcp_conn_t *);
extern void *tcp_conn_userptr(tcp_conn_t *);
extern errno_t tcp_listener_create(tcp_t *, inet_ep_t *, tcp_listen_cb_t *, void *,
    tcp_cb_t *, void *, tcp_listener_t **);
extern void tcp_listener_destroy(tcp_listener_t *);
extern void *tcp_listener_userptr(tcp_listener_t *);

extern errno_t tcp_conn_wait_connected(tcp_conn_t *);
extern errno_t tcp_conn_send(tcp_conn_t *, const void *, size_t);
extern errno_t tcp_conn_send_fin(tcp_conn_t *);
extern errno_t tcp_conn_push(tcp_conn_t *);
extern errno_t tcp_conn_reset(tcp_conn_t *);

extern errno_t tcp_conn_recv(tcp_conn_t *, void *, size_t, size_t *);
extern errno_t tcp_conn_recv_wait(tcp_conn_t *, void *, size_t, size_t *);

#endif

/** @}
 */
