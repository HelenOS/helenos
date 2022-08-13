/*
 * SPDX-FileCopyrightText: 2016 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libinet
 * @{
 */
/** @file
 */

#ifndef LIBINET_TYPES_INET_HOSTPORT_H
#define LIBINET_TYPES_INET_HOSTPORT_H

#include <inet/addr.h>
#include <stdint.h>
#include <types/inet/host.h>

/** Internet host:port specification
 *
 * As in RFC 1738 Uniform Resouce Locators (URL) and RFC 2732 Format for
 * literal IPv6 Addresses in URLs
 */
typedef struct {
	/** Host form */
	inet_host_form_t hform;

	union {
		/** Host name */
		char *name;
		/** Host address */
		inet_addr_t addr;
	} host;

	/** Port number of 0 if omitted */
	uint16_t port;
} inet_hostport_t;

#endif

/** @}
 */
