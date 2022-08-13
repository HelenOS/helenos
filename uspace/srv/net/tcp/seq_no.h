/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tcp
 * @{
 */
/** @file Sequence number computations
 */

#ifndef SEQ_NO_H
#define SEQ_NO_H

#include <stdint.h>
#include "tcp_type.h"

extern bool seq_no_ack_acceptable(tcp_conn_t *, uint32_t);
extern bool seq_no_ack_duplicate(tcp_conn_t *, uint32_t);
extern bool seq_no_in_rcv_wnd(tcp_conn_t *, uint32_t);
extern bool seq_no_new_wnd_update(tcp_conn_t *, tcp_segment_t *);
extern bool seq_no_segment_acked(tcp_conn_t *, tcp_segment_t *, uint32_t);
extern bool seq_no_syn_acked(tcp_conn_t *);
extern bool seq_no_segment_ready(tcp_conn_t *, tcp_segment_t *);
extern bool seq_no_segment_acceptable(tcp_conn_t *, tcp_segment_t *);
extern void seq_no_seg_trim_calc(tcp_conn_t *, tcp_segment_t *, uint32_t *,
    uint32_t *);
extern int seq_no_seg_cmp(tcp_conn_t *, tcp_segment_t *, tcp_segment_t *);

extern uint32_t seq_no_control_len(tcp_control_t);

#endif

/** @}
 */
