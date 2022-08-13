/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup inet
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef ARP_H_
#define ARP_H_

#include <inet/addr.h>
#include <inet/eth_addr.h>
#include <inet/iplink_srv.h>
#include "ethip.h"

extern void arp_received(ethip_nic_t *, eth_frame_t *);
extern errno_t arp_translate(ethip_nic_t *, addr32_t, addr32_t, eth_addr_t *);

#endif

/** @}
 */
