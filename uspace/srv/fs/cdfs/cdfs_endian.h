/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup cdfs
 * @{
 */

#ifndef CDFS_CDFS_ENDIAN_H_
#define CDFS_CDFS_ENDIAN_H_

#include <stdint.h>

#if !(defined(__BE__) ^ defined(__LE__))
#error The architecture must be either big-endian or little-endian.
#endif

typedef struct {
	uint16_t le;
	uint16_t be;
} __attribute__((packed)) uint16_t_lb;

typedef struct {
	uint32_t le;
	uint32_t be;
} __attribute__((packed)) uint32_t_lb;

static inline uint16_t uint16_lb(uint16_t_lb val)
{
#ifdef __LE__
	return val.le;
#endif

#ifdef __BE__
	return val.be;
#endif
}

static inline uint32_t uint32_lb(uint32_t_lb val)
{
#ifdef __LE__
	return val.le;
#endif

#ifdef __BE__
	return val.be;
#endif
}

#endif

/**
 * @}
 */
