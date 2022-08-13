/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tcp
 * @{
 */
/** @file Global segment receive queue
 */

#ifndef RQUEUE_H
#define RQUEUE_H

#include <inet/endpoint.h>
#include "tcp_type.h"

extern void tcp_rqueue_init(tcp_rqueue_cb_t *);
extern void tcp_rqueue_fibril_start(void);
extern void tcp_rqueue_fini(void);
extern void tcp_rqueue_insert_seg(inet_ep2_t *, tcp_segment_t *);

#endif

/** @}
 */
