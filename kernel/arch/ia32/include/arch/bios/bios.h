/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_BIOS_H_
#define KERN_ia32_BIOS_H_

#include <typedefs.h>

extern uintptr_t ebda;

extern void bios_init(void);

#endif /* KERN_ia32_BIOS_H_ */

/** @}
 */
