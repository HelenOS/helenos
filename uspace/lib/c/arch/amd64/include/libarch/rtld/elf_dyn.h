/*
 * SPDX-FileCopyrightText: 2016 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup generic
 * @{
 */
/** @file amd64 dynamic relocation types
 */

#ifndef _LIBC_amd64_RTLD_ELF_DYN_H_
#define _LIBC_amd64_RTLD_ELF_DYN_H_

#define R_X86_64_64		1
#define R_X86_64_PC32		2
#define R_X86_64_COPY		5
#define R_X86_64_GLOB_DAT	6
#define R_X86_64_JUMP_SLOT	7
#define R_X86_64_RELATIVE	8

#define R_X86_64_DTPMOD64	16
#define R_X86_64_DTPOFF64	17
#define R_X86_64_TPOFF64	18

#endif

/** @}
 */
