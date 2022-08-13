/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup generic
 * @{
 */
/** @file sparc64 dynamic relocation types
 */

#ifndef _LIBC_sparc64_RTLD_ELF_DYN_H_
#define _LIBC_sparc64_RTLD_ELF_DYN_H_

#define R_SPARC_COPY		19
#define R_SPARC_GLOB_DAT	20
#define R_SPARC_JMP_SLOT	21
#define R_SPARC_RELATIVE	22
#define R_SPARC_64		32
#define R_SPARC_TLS_DTPMOD64	75
#define R_SPARC_TLS_DTPOFF64	77
#define R_SPARC_TLS_TPOFF64	79

#endif

/** @}
 */
