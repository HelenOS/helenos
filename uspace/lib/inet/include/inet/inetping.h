/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libinet
 * @{
 */
/** @file
 */

#ifndef LIBINET_INET_INETPING_H
#define LIBINET_INET_INETPING_H

#include <inet/inet.h>
#include <types/inetping.h>

typedef struct inetping_ev_ops {
	errno_t (*recv)(inetping_sdu_t *);
} inetping_ev_ops_t;

extern errno_t inetping_init(inetping_ev_ops_t *);
extern errno_t inetping_send(inetping_sdu_t *);
extern errno_t inetping_get_srcaddr(const inet_addr_t *, inet_addr_t *);

#endif

/** @}
 */
