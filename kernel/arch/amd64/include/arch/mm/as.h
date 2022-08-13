/*
 * SPDX-FileCopyrightText: 2005 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_AS_H_
#define KERN_amd64_AS_H_

#define ADDRESS_SPACE_HOLE_START  UINT64_C(0x0000800000000000)
#define ADDRESS_SPACE_HOLE_END    UINT64_C(0xffff7fffffffffff)

#define KERNEL_ADDRESS_SPACE_SHADOWED_ARCH  0
#define KERNEL_SEPARATE_PTL0_ARCH           0

#define KERNEL_ADDRESS_SPACE_START_ARCH  UINT64_C(0xffff800000000000)
#define KERNEL_ADDRESS_SPACE_END_ARCH    UINT64_C(0xffffffffffffffff)
#define USER_ADDRESS_SPACE_START_ARCH    UINT64_C(0x0000000000000000)
#define USER_ADDRESS_SPACE_END_ARCH      UINT64_C(0x00007fffffffffff)

#define as_constructor_arch(as, flags)  ((void)as, (void)flags, EOK)
#define as_destructor_arch(as)          ((void)as, 0)
#define as_create_arch(as, flags)       ((void)as, (void)flags, EOK)

#define as_install_arch(as)
#define as_deinstall_arch(as)
#define as_invalidate_translation_cache(as, page, cnt)

typedef struct {
} as_arch_t;

#include <genarch/mm/as_pt.h>

extern void as_arch_init(void);

#endif

/** @}
 */
