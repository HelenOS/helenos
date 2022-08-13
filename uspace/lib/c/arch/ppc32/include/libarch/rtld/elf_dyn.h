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

#ifndef _LIBC_ppc32_RTLD_ELF_DYN_H_
#define _LIBC_ppc32_RTLD_ELF_DYN_H_

/*
 * ppc32 dynamic relocation types
 */

#define R_PPC_ADDR32   1
#define R_PPC_REL24    10
#define R_PPC_COPY     19
#define R_PPC_JMP_SLOT 21
#define R_PPC_RELATIVE 22

#define R_PPC_DTPMOD32 68
#define R_PPC_DTPREL32 78

#endif

/** @}
 */
