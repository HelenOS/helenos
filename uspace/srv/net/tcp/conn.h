/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
