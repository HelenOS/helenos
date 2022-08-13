/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tcp
 * @{
 */
/** @file TCP inet interfacing
 */

#ifndef TCP_INET_H
#define TCP_INET_H

#include "tcp_type.h"

extern errno_t tcp_inet_init(void);
extern void tcp_transmit_pdu(tcp_pdu_t *);

#endif

/** @}
 */
