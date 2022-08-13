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

#ifndef ETH_PDU_H_
#define ETH_PDU_H_

#include "ethip.h"

extern errno_t eth_pdu_encode(eth_frame_t *, void **, size_t *);
extern errno_t eth_pdu_decode(void *, size_t, eth_frame_t *);
extern errno_t arp_pdu_encode(arp_eth_packet_t *, void **, size_t *);
extern errno_t arp_pdu_decode(void *, size_t, arp_eth_packet_t *);

#endif

/** @}
 */
