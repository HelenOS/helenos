/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 * SPDX-FileCopyrightText: 2013 Antonin Steinhauser
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup inet
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef NTRANS_H_
#define NTRANS_H_

#include <inet/addr.h>
#include <inet/eth_addr.h>
#include <inet/iplink_srv.h>

/** Address translation table element */
typedef struct {
	link_t ntrans_list;
	addr128_t ip_addr;
	eth_addr_t mac_addr;
} inet_ntrans_t;

extern errno_t ntrans_add(addr128_t, eth_addr_t *);
extern errno_t ntrans_remove(addr128_t);
extern errno_t ntrans_lookup(addr128_t, eth_addr_t *);
extern errno_t ntrans_wait_timeout(usec_t);

#endif

/** @}
 */
