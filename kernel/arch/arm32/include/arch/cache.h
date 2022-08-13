/*
 * SPDX-FileCopyrightText: 2013 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32
 * @{
 */
/** @file
 *  @brief Security Extensions Routines
 */

#ifndef KERN_arm32_CACHE_H_
#define KERN_arm32_CACHE_H_

#include <typedefs.h>

extern unsigned dcache_levels(void);

extern void dcache_flush(void);
extern void dcache_flush_invalidate(void);
extern void cpu_dcache_flush(void);
extern void cpu_dcache_flush_invalidate(void);
extern void icache_invalidate(void);
extern void dcache_invalidate(void);
extern void dcache_clean_mva_pou(uintptr_t);

#endif

/** @}
 */
