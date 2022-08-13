/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcia32
 * @{
 */
/** @file
 */

#ifndef _LIBC_ia32_CONFIG_H_
#define _LIBC_ia32_CONFIG_H_

#define PAGE_WIDTH  12
#define PAGE_SIZE   (1 << PAGE_WIDTH)

#define USER_ADDRESS_SPACE_START_ARCH  UINT32_C(0x00000000)
#define USER_ADDRESS_SPACE_END_ARCH    UINT32_C(0x7fffffff)

#endif

/** @}
 */
