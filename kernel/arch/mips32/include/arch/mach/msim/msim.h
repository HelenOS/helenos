/*
 * SPDX-FileCopyrightText: 2013 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 *  @brief msim platform driver.
 */

#ifndef KERN_mips32_msim_H_
#define KERN_mips32_msim_H_

#include <arch/machine_func.h>
#include <arch/mm/page.h>

/** Address of devices. */
#define MSIM_VIDEORAM        PA2KSEG1(0x10000000)
#define MSIM_KBD_ADDRESS     PA2KSEG1(0x10000000)
#define MSIM_DORDER_ADDRESS  PA2KSEG1(0x10000100)

#define MSIM_KBD_IRQ      2
#define MSIM_DORDER_IRQ   5
#define MSIM_DDISK_IRQ    6

extern struct mips32_machine_ops msim_machine_ops;

#endif

/** @}
 */
