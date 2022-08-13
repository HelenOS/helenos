/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libinet
 * @{
 */
/** @file
 */

#ifndef LIBINET_INET_INETCFG_H
#define LIBINET_INET_INETCFG_H

#include <inet/inet.h>
#include <stddef.h>
#include <types/inetcfg.h>

extern errno_t inetcfg_init(void);
extern errno_t inetcfg_addr_create_static(const char *, inet_naddr_t *, sysarg_t, sysarg_t *);
extern errno_t inetcfg_addr_delete(sysarg_t);
extern errno_t inetcfg_addr_get(sysarg_t, inet_addr_info_t *);
extern errno_t inetcfg_addr_get_id(const char *, sysarg_t, sysarg_t *);
extern errno_t inetcfg_get_addr_list(sysarg_t **, size_t *);
extern errno_t inetcfg_get_link_list(sysarg_t **, size_t *);
extern errno_t inetcfg_get_sroute_list(sysarg_t **, size_t *);
extern errno_t inetcfg_link_add(sysarg_t);
extern errno_t inetcfg_link_get(sysarg_t, inet_link_info_t *);
extern errno_t inetcfg_link_remove(sysarg_t);
extern errno_t inetcfg_sroute_get(sysarg_t, inet_sroute_info_t *);
extern errno_t inetcfg_sroute_get_id(const char *, sysarg_t *);
extern errno_t inetcfg_sroute_create(const char *, inet_naddr_t *, inet_addr_t *,
    sysarg_t *);
extern errno_t inetcfg_sroute_delete(sysarg_t);

#endif

/** @}
 */
