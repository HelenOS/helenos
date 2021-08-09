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

#ifndef LIBINET_INET_UDP_H
#define LIBINET_INET_UDP_H

#include <async.h>
#include <fibril_synch.h>
#include <inet/addr.h>
#include <inet/endpoint.h>
#include <inet/inet.h>
#include <stdbool.h>

/** UDP link state */
typedef enum {
	udp_ls_down,
	udp_ls_up
} udp_link_state_t;

/** UDP received message */
typedef struct {
	struct udp *udp;
	sysarg_t assoc_id;
	size_t size;
	inet_ep_t remote_ep;
} udp_rmsg_t;

/** UDP received error */
typedef struct {
} udp_rerr_t;

/** UDP association */
typedef struct {
	struct udp *udp;
	link_t ludp;
	sysarg_t id;
	struct udp_cb *cb;
	void *cb_arg;
} udp_assoc_t;

/** UDP callbacks */
typedef struct udp_cb {
	void (*recv_msg)(udp_assoc_t *, udp_rmsg_t *);
	void (*recv_err)(udp_assoc_t *, udp_rerr_t *);
	void (*link_state)(udp_assoc_t *, udp_link_state_t);
} udp_cb_t;

/** UDP service */
typedef struct udp {
	/** UDP session */
	async_sess_t *sess;
	/** List of associations */
	list_t assoc; /* of udp_assoc_t */
	/** UDP service lock */
	fibril_mutex_t lock;
	/** For waiting on cb_done */
	fibril_condvar_t cv;
	/** Set to @a true when callback connection handler has terminated */
	bool cb_done;
} udp_t;

extern errno_t udp_create(udp_t **);
extern void udp_destroy(udp_t *);
extern errno_t udp_assoc_create(udp_t *, inet_ep2_t *, udp_cb_t *, void *,
    udp_assoc_t **);
extern errno_t udp_assoc_set_nolocal(udp_assoc_t *);
extern void udp_assoc_destroy(udp_assoc_t *);
extern errno_t udp_assoc_send_msg(udp_assoc_t *, inet_ep_t *, void *, size_t);
extern void *udp_assoc_userptr(udp_assoc_t *);
extern size_t udp_rmsg_size(udp_rmsg_t *);
extern errno_t udp_rmsg_read(udp_rmsg_t *, size_t, void *, size_t);
extern void udp_rmsg_remote_ep(udp_rmsg_t *, inet_ep_t *);
extern uint8_t udp_rerr_type(udp_rerr_t *);
extern uint8_t udp_rerr_code(udp_rerr_t *);

#endif

/** @}
 */
