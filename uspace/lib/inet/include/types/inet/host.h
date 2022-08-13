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

#ifndef LIBINETTYPES_INET_HOST_H
#define LIBINETTYPES_INET_HOST_H

#include <inet/addr.h>

typedef enum {
	/** Host name */
	inet_host_name,
	/** Host address */
	inet_host_addr
} inet_host_form_t;

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
} inet_host_t;

#endif

/** @}
 */
