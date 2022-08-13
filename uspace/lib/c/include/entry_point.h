/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_ENTRY_POINT_H_
#define _LIBC_ENTRY_POINT_H_

/* Defined in arch/ARCH/src/entryjmp.[c|s] */
void entry_point_jmp(void *, void *) __attribute__((noreturn));

#endif

/** @}
 */
