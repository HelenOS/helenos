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

#ifndef LIBINET_INET_HOST_H
#define LIBINET_INET_HOST_H

#include <inet/addr.h>
#include <inet/endpoint.h>
#include <types/inet/hostport.h>

extern errno_t inet_host_parse(const char *, inet_host_t **, char **);
extern errno_t inet_host_format(inet_host_t *, char **);
extern void inet_host_destroy(inet_host_t *);
extern errno_t inet_host_lookup_one(inet_host_t *, ip_ver_t, inet_addr_t *);
extern errno_t inet_host_plookup_one(const char *, ip_ver_t, inet_addr_t *,
    char **, const char **);

#endif

/** @}
 */
