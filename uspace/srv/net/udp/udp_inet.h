/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup udp
 * @{
 */
/** @file
 */

#ifndef UDP_INET_H
#define UDP_INET_H

#include <inet/addr.h>
#include <stdint.h>
#include "udp_type.h"

extern errno_t udp_inet_init(void);
extern errno_t udp_transmit_pdu(udp_pdu_t *);
extern errno_t udp_get_srcaddr(inet_addr_t *, uint8_t, inet_addr_t *);
extern errno_t udp_transmit_msg(inet_ep2_t *, udp_msg_t *);

#endif

/** @}
 */
