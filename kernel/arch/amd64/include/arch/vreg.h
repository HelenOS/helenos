/*
 * SPDX-FileCopyrightText: 2016 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_VREG_H_
#define KERN_amd64_VREG_H_

#define VREG_TP	0

#ifndef __ASSEMBLER__

#include <stdint.h>

extern uint64_t *vreg_ptr;

extern void vreg_init(void);

#endif /* __ASSEMBLER__ */

#endif

/** @}
 */
