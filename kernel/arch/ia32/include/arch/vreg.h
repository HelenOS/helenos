/*
 * SPDX-FileCopyrightText: 2016 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia32
 * @{
 */
/** @file
 */

#ifndef KERN_ia32_VREG_H_
#define KERN_ia32_VREG_H_

#define VREG_TP	0

#ifndef __ASSEMBLER__

#include <stdint.h>

extern uint32_t *vreg_ptr;

extern void vreg_init(void);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
