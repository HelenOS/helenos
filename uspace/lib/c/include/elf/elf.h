/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_ELF_H_
#define _LIBC_ELF_H_

#include <stdint.h>
#include <types/common.h>
#include <abi/elf.h>

extern const elf_segment_header_t *elf_get_phdr(const void *, unsigned);
extern uintptr_t elf_get_bias(const void *);

#endif

/** @}
 */
