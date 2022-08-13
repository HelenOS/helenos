/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tcp
 * @{
 */
/** @file TCP user calls (close to those defined in the RFC)
 */

#ifndef UCALL_H
#define UCALL_H

#include <inet/endpoint.h>
#include <stddef.h>
#include "tcp_type.h"

/*
 * User calls
 */
extern tcp_error_t tcp_uc_open(inet_ep2_t *, acpass_t,
    tcp_open_flags_t, tcp_conn_t **);
extern tcp_error_t tcp_uc_send(tcp_conn_t *, void *, size_t, xflags_t);
extern tcp_error_t tcp_uc_receive(tcp_conn_t *, void *, size_t, size_t *, xflags_t *);
extern tcp_error_t tcp_uc_close(tcp_conn_t *);
extern void tcp_uc_abort(tcp_conn_t *);
extern void tcp_uc_status(tcp_conn_t *, tcp_conn_status_t *);
extern void tcp_uc_delete(tcp_conn_t *);
extern void tcp_uc_set_cb(tcp_conn_t *, tcp_cb_t *, void *);
extern void *tcp_uc_get_userptr(tcp_conn_t *);

/*
 * Arriving segments
 */
extern void tcp_as_segment_arrived(inet_ep2_t *, tcp_segment_t *);

/*
 * Timeouts
 */
extern void tcp_to_user(void);

#endif

/** @}
 */
