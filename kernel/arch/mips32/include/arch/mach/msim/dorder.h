/*
 * SPDX-FileCopyrightText: 2007 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_DORDER_H_
#define KERN_mips32_DORDER_H_

#include <stdint.h>

extern void dorder_init(void);
extern uint32_t dorder_cpuid(void);
extern void dorder_ipi_ack(uint32_t);

#endif

/** @}
 */
