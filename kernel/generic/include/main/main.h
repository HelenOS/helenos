/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_MAIN_H_
#define KERN_MAIN_H_

#include <typedefs.h>

/* Address of the start of the kernel image. */
extern uint8_t kernel_load_address[];
/* Address of the end of kernel. */
extern uint8_t kdata_end[];

extern uintptr_t stack_safe;

extern void main_bsp(void);
extern void main_ap(void);

extern void malloc_init(void);

#endif

/** @}
 */
