/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup dnsrsrv
 * @{
 */
/**
 * @file
 */

#ifndef QUERY_H
#define QUERY_H

#include <inet/addr.h>
#include "dns_type.h"

extern errno_t dns_name2host(const char *, dns_host_info_t **, ip_ver_t);
extern void dns_hostinfo_destroy(dns_host_info_t *);

#endif

/** @}
 */
