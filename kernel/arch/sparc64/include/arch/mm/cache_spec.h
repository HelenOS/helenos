/*
 * SPDX-FileCopyrightText: 2008 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_CACHE_SPEC_H_
#define KERN_sparc64_CACHE_SPEC_H_

/*
 * The following macros are valid for the following processors:
 *
 *  UltraSPARC, UltraSPARC II, UltraSPARC IIi, UltraSPARC III,
 *  UltraSPARC III+, UltraSPARC IV, UltraSPARC IV+
 *
 * Should we support other UltraSPARC processors, we need to make sure that
 * the macros are defined correctly for them.
 */

#if defined (US)
#define DCACHE_SIZE  (16 * 1024)
#elif defined (US3)
#define DCACHE_SIZE  (64 * 1024)
#endif

#define DCACHE_LINE_SIZE  32
#define DCACHE_TAG_SHIFT  2

#endif

/** @}
 */
