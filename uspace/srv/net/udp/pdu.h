/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup udp
 * @{
 */
/** @file UDP PDU (encoded Protocol Data Unit) handling
 */

#ifndef PDU_H
#define PDU_H

#include <inet/endpoint.h>
#include "std.h"
#include "udp_type.h"

extern udp_pdu_t *udp_pdu_new(void);
extern void udp_pdu_delete(udp_pdu_t *);
extern errno_t udp_pdu_decode(udp_pdu_t *, inet_ep2_t *, udp_msg_t **);
extern errno_t udp_pdu_encode(inet_ep2_t *, udp_msg_t *, udp_pdu_t **);

#endif

/** @}
 */
