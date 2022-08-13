/*
 * SPDX-FileCopyrightText: 2013 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 *  @brief MIPS Malta platform driver.
 */

#ifndef KERN_mips32_malta_H_
#define KERN_mips32_malta_H_

#include <arch/machine_func.h>
#include <arch/mm/page.h>
#include <typedefs.h>

#define MALTA_PCI_PHYSBASE	0x18000000UL

#define MALTA_PCI_BASE		PA2KSEG1(MALTA_PCI_PHYSBASE)
#define MALTA_GT64120_BASE	PA2KSEG1(0x1be00000UL)

#define PIC0_BASE		(MALTA_PCI_BASE + 0x20)
#define PIC1_BASE		(MALTA_PCI_BASE + 0xa0)

#define ISA_IRQ_COUNT		16

#define TTY_BASE		(MALTA_PCI_PHYSBASE + 0x3f8)
#define TTY_ISA_IRQ		4

#define GT64120_PCI0_INTACK	((ioport32_t *) (MALTA_GT64120_BASE + 0xc34))

extern struct mips32_machine_ops malta_machine_ops;

#endif

/** @}
 */
