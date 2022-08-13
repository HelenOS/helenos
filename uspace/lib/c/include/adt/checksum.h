/*
 * SPDX-FileCopyrightText: 2012 Dominik Taborsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_CHECKSUM_H_
#define _LIBC_CHECKSUM_H_

#include <stddef.h>
#include <stdint.h>

extern uint32_t compute_crc32(uint8_t *, size_t);
extern uint32_t compute_crc32_seed(uint8_t *, size_t, uint32_t);

#endif

/** @}
 */
