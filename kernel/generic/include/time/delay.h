/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_time
 * @{
 */
/** @file
 */

#ifndef KERN_DELAY_H_
#define KERN_DELAY_H_

#include <stdint.h>

extern void delay(uint32_t microseconds);

#endif

/** @}
 */
