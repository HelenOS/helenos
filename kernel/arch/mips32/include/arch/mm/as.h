/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32_mm
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_AS_H_
#define KERN_mips32_AS_H_

#define KERNEL_ADDRESS_SPACE_SHADOWED_ARCH  0
#define KERNEL_SEPARATE_PTL0_ARCH           0

#define KERNEL_ADDRESS_SPACE_START_ARCH  UINT32_C(0x80000000)
#define KERNEL_ADDRESS_SPACE_END_ARCH    UINT32_C(0xffffffff)
#define USER_ADDRESS_SPACE_START_ARCH    UINT32_C(0x00000000)
#define USER_ADDRESS_SPACE_END_ARCH      UINT32_C(0x7fffffff)

typedef struct {
} as_arch_t;

#include <genarch/mm/as_pt.h>

#define as_constructor_arch(as, flags)  ((void)as, (void)flags, EOK)
#define as_destructor_arch(as)          ((void)as, 0)
#define as_create_arch(as, flags)       ((void)as, (void)flags, EOK)
#define as_deinstall_arch(as)
#define as_invalidate_translation_cache(as, page, cnt)

extern void as_arch_init(void);

#endif

/** @}
 */
