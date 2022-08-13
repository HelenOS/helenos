/*
 * SPDX-FileCopyrightText: 2017 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_riscv64
 * @{
 */
/** @file
 */

#ifndef KERN_riscv64_DRIVERS_UCB_H_
#define KERN_riscv64_DRIVERS_UCB_H_

#include <stddef.h>
#include <stdint.h>
#include <console/chardev.h>

extern void htif_init(volatile uint64_t *, volatile uint64_t *);
extern outdev_t *htifout_init(void);
extern void htif_putuchar(outdev_t *, const char32_t);

#endif

/** @}
 */
