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

#ifndef ATRANS_H_
#define ATRANS_H_

#include <inet/addr.h>
#include <inet/eth_addr.h>
#include <inet/iplink_srv.h>
#include "ethip.h"

extern errno_t atrans_add(addr32_t, eth_addr_t *);
extern errno_t atrans_remove(addr32_t);
extern errno_t atrans_lookup(addr32_t, eth_addr_t *);
extern errno_t atrans_lookup_timeout(addr32_t, usec_t, eth_addr_t *);

#endif

/** @}
 */
