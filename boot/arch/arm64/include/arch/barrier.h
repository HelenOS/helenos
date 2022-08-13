/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup boot_arm64
 * @{
 */
/** @file
 * @brief Memory barriers.
 */

#ifndef BOOT_arm64_BARRIER_H_
#define BOOT_arm64_BARRIER_H_

#include <stddef.h>

extern void smc_coherence(void *, size_t);
extern void dcache_flush(void *, size_t);

#endif

/** @}
 */
