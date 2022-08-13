/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
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

#ifndef INET_PDU_H_
#define INET_PDU_H_

#include <loc.h>
#include <stddef.h>
#include <stdint.h>
#include "inetsrv.h"
#include "ndp.h"

#define INET_CHECKSUM_INIT 0xffff

extern uint16_t inet_checksum_calc(uint16_t, void *, size_t);

extern errno_t inet_pdu_encode(inet_packet_t *, addr32_t, addr32_t, size_t, size_t,
    void **, size_t *, size_t *);
extern errno_t inet_pdu_encode6(inet_packet_t *, addr128_t, addr128_t, size_t,
    size_t, void **, size_t *, size_t *);
extern errno_t inet_pdu_decode(void *, size_t, service_id_t, inet_packet_t *);
extern errno_t inet_pdu_decode6(void *, size_t, service_id_t, inet_packet_t *);

extern errno_t ndp_pdu_decode(inet_dgram_t *, ndp_packet_t *);
extern errno_t ndp_pdu_encode(ndp_packet_t *, inet_dgram_t *);

#endif

/** @}
 */
