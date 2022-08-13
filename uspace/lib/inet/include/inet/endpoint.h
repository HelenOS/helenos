/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libinet
 * @{
 */
/** @file
 */

#ifndef LIBINET_INET_ENDPOINT_H
#define LIBINET_INET_ENDPOINT_H

#include <stdint.h>
#include <inet/addr.h>
#include <loc.h>

/** Internet port number ranges
 *
 * Port number ranges per RFC 6335 section 6 (Port Number Ranges.
 * Technically port zero is a system port. But since it is reserved,
 * we will use it as a special value denoting no port is specified
 * and we will exclude it from the system port range to disallow
 * ever assigning it.
 */
enum inet_port_ranges {
	/** Special value meaning no specific port */
	inet_port_any = 0,
	/** Lowest system port (a.k.a. well known port) */
	inet_port_sys_lo = 1,
	/** Highest system port (a.k.a. well known port) */
	inet_port_sys_hi = 1023,
	/** Lowest user port (a.k.a. registered port) */
	inet_port_user_lo = 1024,
	/** Highest user port (a.k.a. registered port) */
	inet_port_user_hi = 49151,
	/** Lowest dynamic port (a.k.a. private or ephemeral port) */
	inet_port_dyn_lo = 49152,
	/** Highest dynamic port (a.k.a. private or ephemeral port) */
	inet_port_dyn_hi = 65535
};

/** Internet endpoint (address-port pair), a.k.a. socket */
typedef struct {
	inet_addr_t addr;
	uint16_t port;
} inet_ep_t;

/** Internet endpoint pair */
typedef struct {
	service_id_t local_link;
	inet_ep_t local;
	inet_ep_t remote;
} inet_ep2_t;

extern void inet_ep_init(inet_ep_t *);
extern void inet_ep2_init(inet_ep2_t *);

#endif

/** @}
 */
