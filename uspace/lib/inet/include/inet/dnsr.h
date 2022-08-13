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

#ifndef LIBINET_INET_DNSR_H
#define LIBINET_INET_DNSR_H

#include <inet/inet.h>
#include <inet/addr.h>

enum {
	DNSR_NAME_MAX_SIZE = 255
};

typedef struct {
	/** Host canonical name */
	char *cname;
	/** Host address */
	inet_addr_t addr;
} dnsr_hostinfo_t;

extern errno_t dnsr_init(void);
extern errno_t dnsr_name2host(const char *, dnsr_hostinfo_t **, ip_ver_t);
extern void dnsr_hostinfo_destroy(dnsr_hostinfo_t *);
extern errno_t dnsr_get_srvaddr(inet_addr_t *);
extern errno_t dnsr_set_srvaddr(inet_addr_t *);

#endif

/** @}
 */
