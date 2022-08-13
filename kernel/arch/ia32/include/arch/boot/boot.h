/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_BOOT_H_
#define KERN_ia32_BOOT_H_

#define BOOT_OFFSET      0x108000
#define AP_BOOT_OFFSET   0x8000
#define BOOT_STACK_SIZE  0x0400

#ifndef __ASSEMBLER__

#ifdef CONFIG_SMP

extern uint8_t unmapped_end[];

#endif /* CONFIG_SMP */

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
