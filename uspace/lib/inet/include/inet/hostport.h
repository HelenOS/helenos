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

#ifndef LIBINET_INET_HOSTPORT_H
#define LIBINET_INET_HOSTPORT_H

#include <inet/addr.h>
#include <inet/endpoint.h>
#include <types/inet/hostport.h>

extern errno_t inet_hostport_parse(const char *, inet_hostport_t **, char **);
extern errno_t inet_hostport_format(inet_hostport_t *, char **);
extern void inet_hostport_destroy(inet_hostport_t *);
extern errno_t inet_hostport_lookup_one(inet_hostport_t *, ip_ver_t, inet_ep_t *);
extern errno_t inet_hostport_plookup_one(const char *, ip_ver_t, inet_ep_t *,
    char **, const char **);

#endif

/** @}
 */
