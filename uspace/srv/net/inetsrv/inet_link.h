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

#ifndef INET_LINK_H_
#define INET_LINK_H_

#include <inet/eth_addr.h>
#include <stddef.h>
#include <stdint.h>
#include "inetsrv.h"

extern errno_t inet_link_open(service_id_t);
extern errno_t inet_link_send_dgram(inet_link_t *, addr32_t,
    addr32_t, inet_dgram_t *, uint8_t, uint8_t, int);
extern errno_t inet_link_send_dgram6(inet_link_t *, eth_addr_t *, inet_dgram_t *,
    uint8_t, uint8_t, int);
extern inet_link_t *inet_link_get_by_id(sysarg_t);
extern errno_t inet_link_get_id_list(sysarg_t **, size_t *);

#endif

/** @}
 */
