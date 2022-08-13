/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tcp
 * @{
 */
/** @file Connection incoming segments queue
 */

#ifndef IQUEUE_H
#define IQUEUE_H

#include "tcp_type.h"

extern void tcp_iqueue_init(tcp_iqueue_t *, tcp_conn_t *);
extern void tcp_iqueue_insert_seg(tcp_iqueue_t *, tcp_segment_t *);
extern void tcp_iqueue_remove_seg(tcp_iqueue_t *, tcp_segment_t *);
extern errno_t tcp_iqueue_get_ready_seg(tcp_iqueue_t *, tcp_segment_t **);

#endif

/** @}
 */
