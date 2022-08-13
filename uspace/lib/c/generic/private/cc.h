/*
 * SPDX-FileCopyrightText: 2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 * Macros related to used compiler (such as GCC-specific attributes).
 */

#ifndef _LIBC_CC_H_
#define _LIBC_CC_H_

#ifndef __clang__
#define ATTRIBUTE_OPTIMIZE(opt) \
    __attribute__ ((optimize(opt)))
#else
#define ATTRIBUTE_OPTIMIZE(opt)
#endif

#define ATTRIBUTE_OPTIMIZE_NO_TLDP \
    ATTRIBUTE_OPTIMIZE("-fno-tree-loop-distribute-patterns")

#endif

/** @}
 */
