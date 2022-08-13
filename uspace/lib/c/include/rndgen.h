/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file Random number generator
 */

#ifndef _LIBC_RNDGEN_H_
#define _LIBC_RNDGEN_H_

#include <errno.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
	unsigned int seed;
} rndgen_t;

extern errno_t rndgen_create(rndgen_t **);
extern void rndgen_destroy(rndgen_t *);
extern errno_t rndgen_uint8(rndgen_t *, uint8_t *);
extern errno_t rndgen_uint32(rndgen_t *, uint32_t *);

#endif

/** @}
 */
