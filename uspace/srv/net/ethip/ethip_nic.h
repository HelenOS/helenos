/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup ethip
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef ETHIP_NIC_H_
#define ETHIP_NIC_H_

#include <ipc/loc.h>
#include <inet/addr.h>
#include "ethip.h"

extern errno_t ethip_nic_discovery_start(void);
extern ethip_nic_t *ethip_nic_find_by_iplink_sid(service_id_t);
extern errno_t ethip_nic_send(ethip_nic_t *, void *, size_t);
extern errno_t ethip_nic_addr_add(ethip_nic_t *, inet_addr_t *);
extern errno_t ethip_nic_addr_remove(ethip_nic_t *, inet_addr_t *);
extern ethip_link_addr_t *ethip_nic_addr_find(ethip_nic_t *, inet_addr_t *);

#endif

/** @}
 */
