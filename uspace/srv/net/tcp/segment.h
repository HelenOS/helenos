/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tcp
 * @{
 */
/** @file Segment processing
 */

#ifndef SEGMENT_H
#define SEGMENT_H

#include <stddef.h>
#include <stdint.h>
#include "tcp_type.h"

extern void tcp_segment_delete(tcp_segment_t *);
extern tcp_segment_t *tcp_segment_dup(tcp_segment_t *);
extern tcp_segment_t *tcp_segment_make_ctrl(tcp_control_t);
extern tcp_segment_t *tcp_segment_make_rst(tcp_segment_t *);
extern tcp_segment_t *tcp_segment_make_data(tcp_control_t, void *, size_t);
extern void tcp_segment_trim(tcp_segment_t *, uint32_t, uint32_t);
extern void tcp_segment_text_copy(tcp_segment_t *, void *, size_t);
extern size_t tcp_segment_text_size(tcp_segment_t *);
extern void tcp_segment_dump(tcp_segment_t *);

#endif

/** @}
 */
