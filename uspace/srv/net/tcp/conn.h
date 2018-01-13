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

/** @addtogroup tcp
 * @{
 */
/** @file TCP connection processing and state machine
 */

#ifndef CONN_H
#define CONN_H

#include <inet/endpoint.h>
#include <stdbool.h>
#include "tcp_type.h"

extern errno_t tcp_conns_init(void);
extern void tcp_conns_fini(void);
extern tcp_conn_t *tcp_conn_new(inet_ep2_t *);
extern void tcp_conn_delete(tcp_conn_t *);
extern errno_t tcp_conn_add(tcp_conn_t *);
extern void tcp_conn_reset(tcp_conn_t *conn);
extern void tcp_conn_sync(tcp_conn_t *);
extern void tcp_conn_fin_sent(tcp_conn_t *);
extern tcp_conn_t *tcp_conn_find_ref(inet_ep2_t *);
extern void tcp_conn_addref(tcp_conn_t *);
extern void tcp_conn_delref(tcp_conn_t *);
extern void tcp_conn_lock(tcp_conn_t *);
extern void tcp_conn_unlock(tcp_conn_t *);
extern bool tcp_conn_got_syn(tcp_conn_t *);
extern void tcp_conn_segment_arrived(tcp_conn_t *, inet_ep2_t *,
    tcp_segment_t *);
extern void tcp_unexpected_segment(inet_ep2_t *, tcp_segment_t *);
extern void tcp_ep2_flipped(inet_ep2_t *, inet_ep2_t *);

extern tcp_lb_t tcp_conn_lb;

#endif

/** @}
 */
