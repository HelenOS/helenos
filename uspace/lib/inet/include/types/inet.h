/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libinet
 * @{
 */
/** @file
 */

#ifndef LIBINET_TYPES_INET_H
#define LIBINET_TYPES_INET_H

#include <inet/addr.h>
#include <ipc/loc.h>
#include <stddef.h>
#include <stdint.h>

#define INET_TTL_MAX 255

typedef struct {
	/** Local IP link service ID (optional) */
	service_id_t iplink;
	inet_addr_t src;
	inet_addr_t dest;
	uint8_t tos;
	void *data;
	size_t size;
} inet_dgram_t;

typedef struct {
	errno_t (*recv)(inet_dgram_t *);
} inet_ev_ops_t;

typedef enum {
	INET_DF = 1
} inet_df_t;

#endif

/** @}
 */
