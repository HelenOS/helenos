/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libinet
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef LIBINET_TYPES_INETPING_H
#define LIBINET_TYPES_INETPING_H

#include <inet/addr.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
	inet_addr_t src;
	inet_addr_t dest;
	uint16_t seq_no;
	void *data;
	size_t size;
} inetping_sdu_t;

#endif

/** @}
 */
