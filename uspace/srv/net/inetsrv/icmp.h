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

#ifndef ICMP_H_
#define ICMP_H_

#include <types/inetping.h>
#include "inetsrv.h"

extern errno_t icmp_recv(inet_dgram_t *);
extern errno_t icmp_ping_send(uint16_t, inetping_sdu_t *);

#endif

/** @}
 */
