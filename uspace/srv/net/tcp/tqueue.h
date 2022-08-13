/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tcp
 * @{
 */
/** @file TCP transmission queue
 */

#ifndef TQUEUE_H
#define TQUEUE_H

#include <inet/endpoint.h>
#include "std.h"
#include "tcp_type.h"

extern errno_t tcp_tqueue_init(tcp_tqueue_t *, tcp_conn_t *,
    tcp_tqueue_cb_t *);
extern void tcp_tqueue_clear(tcp_tqueue_t *);
extern void tcp_tqueue_fini(tcp_tqueue_t *);
extern void tcp_tqueue_ctrl_seg(tcp_conn_t *, tcp_control_t);
extern void tcp_tqueue_new_data(tcp_conn_t *);
extern void tcp_tqueue_ack_received(tcp_conn_t *);

#endif

/** @}
 */
