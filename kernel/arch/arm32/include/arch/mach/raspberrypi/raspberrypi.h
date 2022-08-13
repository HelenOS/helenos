/*
 * SPDX-FileCopyrightText: 2013 Beniamino Galvani
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32_raspberrypi raspberrypi
 *  @brief Raspberry Pi platform.
 *  @ingroup kernel_arm32
 * @{
 */
/** @file
 *  @brief Raspberry Pi platform driver.
 */

#ifndef KERN_arm32_raspberrypi_H_
#define KERN_arm32_raspberrypi_H_

#include <arch/machine_func.h>

extern struct arm_machine_ops raspberrypi_machine_ops;

#define BCM2835_UART0_BASE_ADDRESS   0x20201000

#endif

/** @}
 */
