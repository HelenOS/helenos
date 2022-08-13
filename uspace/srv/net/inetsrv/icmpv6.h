/*
 * SPDX-FileCopyrightText: 2013 Antonin Steinhauser
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

#ifndef ICMPV6_H_
#define ICMPV6_H_

#include <types/inetping.h>
#include "inetsrv.h"

extern errno_t icmpv6_recv(inet_dgram_t *);
extern errno_t icmpv6_ping_send(uint16_t, inetping_sdu_t *);

#endif

/** @}
 */
