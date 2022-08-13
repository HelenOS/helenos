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

#ifndef LIBINET_INET_DHCP_H
#define LIBINET_INET_DHCP_H

#include <types/common.h>

extern errno_t dhcp_init(void);
extern errno_t dhcp_link_add(sysarg_t);
extern errno_t dhcp_link_remove(sysarg_t);
extern errno_t dhcp_discover(sysarg_t);

#endif

/** @}
 */
