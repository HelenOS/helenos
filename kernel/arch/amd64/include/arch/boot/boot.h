/*
 * SPDX-FileCopyrightText: 2005 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_BOOT_H_
#define KERN_amd64_BOOT_H_

#define BOOT_OFFSET      0x108000
#define AP_BOOT_OFFSET   0x008000
#define BOOT_STACK_SIZE  0x000400

#ifndef __ASSEMBLER__

extern uint8_t unmapped_end[];

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
