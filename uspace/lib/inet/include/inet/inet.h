/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libinet
 * @{
 */
/** @file
 */

#ifndef LIBINET_INET_INET_H
#define LIBINET_INET_INET_H

#include <inet/addr.h>
#include <ipc/loc.h>
#include <stdint.h>
#include <types/inet.h>

extern errno_t inet_init(uint8_t, inet_ev_ops_t *);
extern errno_t inet_send(inet_dgram_t *, uint8_t, inet_df_t);
extern errno_t inet_get_srcaddr(inet_addr_t *, uint8_t, inet_addr_t *);

#endif

/** @}
 */
