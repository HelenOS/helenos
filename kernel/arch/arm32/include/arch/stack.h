/*
 * SPDX-FileCopyrightText: 2007 Michal Kebrt
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32
 * @{
 */
/** @file
 *  @brief Stack constants.
 */

#ifndef KERN_arm32_STACK_H_
#define KERN_arm32_STACK_H_

#include <config.h>

#define MEM_STACK_SIZE	STACK_SIZE

#define STACK_ITEM_SIZE		4

/** See <a href="http://www.arm.com/support/faqdev/14269.html">ABI</a> for
 * details
 */
#define STACK_ALIGNMENT		8

#endif

/** @}
 */
