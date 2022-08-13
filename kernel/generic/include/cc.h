/*
 * SPDX-FileCopyrightText: 2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 * Macros related to used compiler (such as GCC-specific attributes).
 */

#ifndef KERN_CC_H_
#define KERN_CC_H_

#ifndef __clang__
#define ATTRIBUTE_OPTIMIZE(opt) \
	__attribute__ ((optimize(opt)))
#define ATTRIBUTE_USED \
	__attribute__ ((used))
#else
#define ATTRIBUTE_OPTIMIZE(opt)
#define ATTRIBUTE_USED
#endif

#endif

/** @}
 */
