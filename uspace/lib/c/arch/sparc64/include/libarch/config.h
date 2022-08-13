/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcsparc64
 * @{
 */
/** @file
 */

#ifndef _LIBC_sparc64_CONFIG_H_
#define _LIBC_sparc64_CONFIG_H_

#if defined (SUN4U)
#define PAGE_WIDTH	14
#elif defined(SUN4V)
#define PAGE_WIDTH	13
#endif

#define PAGE_SIZE	(1 << PAGE_WIDTH)

#endif

/** @}
 */
