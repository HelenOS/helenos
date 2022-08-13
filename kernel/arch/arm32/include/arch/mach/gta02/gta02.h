/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32_gta02 GTA02
 *  @brief Openmoko GTA02 platform.
 *  @ingroup kernel_arm32
 * @{
 */
/** @file
 *  @brief Openmoko GTA02 platform driver.
 */

#ifndef KERN_arm32_gta02_H_
#define KERN_arm32_gta02_H_

#include <arch/machine_func.h>

extern struct arm_machine_ops gta02_machine_ops;

/** Size of GTA02 IRQ number range (starting from 0) */
#define GTA02_IRQ_COUNT 32

#endif

/** @}
 */
