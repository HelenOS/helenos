/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_IT_H_
#define KERN_ia64_IT_H_

/*
 * Unfortunately, Ski does not emulate PAL,
 * so we can't read the real frequency ratios
 * from firmware.
 *
 */
#define IT_DELTA        it_delta

extern void it_init(void);

#endif

/** @}
 */
