/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_ia32_RTLD_MODULE_H_
#define _LIBC_ia32_RTLD_MODULE_H_

#include <elf/elf_mod.h>

/** ELF module load flags.
 *
 * Keep code segment read-write
 */
#define RTLD_MODULE_LDF ELDF_RW

#endif

/** @}
 */
