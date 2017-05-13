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
