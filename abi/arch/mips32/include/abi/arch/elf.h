/*
 * SPDX-FileCopyrightText: 2006 Sergey Bondari
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup abi_mips32
 * @{
 */
/** @file
 */

#ifndef _ABI_mips32_ELF_H_
#define _ABI_mips32_ELF_H_

#define ELF_MACHINE  EM_MIPS

#ifdef __BE__
#define ELF_DATA_ENCODING  ELFDATA2MSB
#else
#define ELF_DATA_ENCODING  ELFDATA2LSB
#endif

#define ELF_CLASS  ELFCLASS32

#endif

/** @}
 */
