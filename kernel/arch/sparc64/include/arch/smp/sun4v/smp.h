/*
 * SPDX-FileCopyrightText: 2009 Pavel Rimsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/**
 * @file
 * @brief	sun4v smp functions
 */

#ifndef KERN_sparc64_sun4v_SMP_H_
#define KERN_sparc64_sun4v_SMP_H_

#include <arch/sun4v/cpu.h>
#include <stdint.h>

extern bool calculate_optimal_nrdy(exec_unit_t *);

#endif

/** @}
 */
