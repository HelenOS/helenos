/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_ia32_RTLD_ELF_DYN_H_
#define _LIBC_ia32_RTLD_ELF_DYN_H_

/*
 * ia32 dynamic relocation types
 */

#define R_386_32	1
#define R_386_PC32	2
#define R_386_COPY	5
#define R_386_GLOB_DAT	6
#define R_386_JUMP_SLOT	7
#define R_386_RELATIVE	8

#define R_386_TLS_TPOFF    14
#define R_386_TLS_DTPMOD32 35
#define R_386_TLS_DTPOFF32 36

#endif

/** @}
 */
