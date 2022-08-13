/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tcp
 * @{
 */
/** @file Network condition simulator
 */

#ifndef NCSIM_H
#define NCSIM_H

#include <inet/endpoint.h>
#include "tcp_type.h"

extern void tcp_ncsim_init(void);
extern void tcp_ncsim_bounce_seg(inet_ep2_t *, tcp_segment_t *);
extern void tcp_ncsim_fibril_start(void);

#endif

/** @}
 */
