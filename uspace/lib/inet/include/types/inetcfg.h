/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libinet
 * @{
 */
/** @file
 */

#ifndef LIBINET_TYPES_INETCFG_H
#define LIBINET_TYPES_INETCFG_H

#include <inet/eth_addr.h>
#include <inet/inet.h>
#include <stddef.h>

/** Address object info */
typedef struct {
	/** Network address */
	inet_naddr_t naddr;
	/** Link service ID */
	sysarg_t ilink;
	/** Address object name */
	char *name;
} inet_addr_info_t;

/** IP link info */
typedef struct {
	/** Link service name */
	char *name;
	/** Default MTU */
	size_t def_mtu;
	/** Link layer address */
	eth_addr_t mac_addr;
} inet_link_info_t;

/** Static route info */
typedef struct {
	/** Destination network address */
	inet_naddr_t dest;
	/** Router address */
	inet_addr_t router;
	/** Static route name */
	char *name;
} inet_sroute_info_t;

#endif

/** @}
 */
