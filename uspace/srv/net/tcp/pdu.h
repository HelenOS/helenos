/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tcp
 * @{
 */
/** @file TCP PDU (encoded Protocol Data Unit) handling
 */

#ifndef PDU_H
#define PDU_H

#include <inet/endpoint.h>
#include <stddef.h>
#include "std.h"
#include "tcp_type.h"

extern tcp_pdu_t *tcp_pdu_create(void *, size_t, void *, size_t);
extern void tcp_pdu_delete(tcp_pdu_t *);
extern errno_t tcp_pdu_decode(tcp_pdu_t *, inet_ep2_t *, tcp_segment_t **);
extern errno_t tcp_pdu_encode(inet_ep2_t *, tcp_segment_t *, tcp_pdu_t **);

#endif

/** @}
 */
