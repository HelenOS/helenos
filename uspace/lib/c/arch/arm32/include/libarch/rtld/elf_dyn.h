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

#ifndef _LIBC_arm32_RTLD_ELF_DYN_H_
#define _LIBC_arm32_RTLD_ELF_DYN_H_

/*
 * arm32 dynamic relocation types
 */

#define R_ARM_ABS32         2
#define R_ARM_TLS_DTPMOD32 17
#define R_ARM_TLS_DTPOFF32 18
#define R_ARM_TLS_TPOFF32  19
#define R_ARM_COPY         20
#define R_ARM_GLOB_DAT     21
#define R_ARM_JUMP_SLOT    22
#define R_ARM_RELATIVE     23

#endif

/** @}
 */
