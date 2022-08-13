/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_I8254_H_
#define KERN_ia32_I8254_H_

extern void i8254_init(void);
extern void i8254_calibrate_delay_loop(void);
extern void i8254_normal_operation(void);

#endif

/** @}
 */
