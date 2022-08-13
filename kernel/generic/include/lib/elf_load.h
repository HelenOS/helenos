/*
 * SPDX-FileCopyrightText: 2006 Sergey Bondari
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_ELF_LOAD_H_
#define KERN_ELF_LOAD_H_

#include <abi/elf.h>

extern errno_t elf_load(elf_header_t *, as_t *);

#endif

/** @}
 */
