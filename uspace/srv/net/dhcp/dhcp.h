/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup dhcp
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef DHCP_H
#define DHCP_H

#include <ipc/loc.h>

extern void dhcpsrv_links_init(void);
extern errno_t dhcpsrv_link_add(service_id_t);
extern errno_t dhcpsrv_link_remove(service_id_t);
extern errno_t dhcpsrv_discover(service_id_t);

#endif

/** @}
 */
