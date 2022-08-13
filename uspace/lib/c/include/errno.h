/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_ERRNO_H_
#define _LIBC_ERRNO_H_

#include <_bits/errno.h>
#include <abi/errno.h>
#include <_bits/decls.h>

__HELENOS_DECLS_BEGIN;

extern errno_t *__errno(void) __attribute__((const));

__HELENOS_DECLS_END;

#ifdef __cplusplus
#define errno  (*(::helenos::__errno()))
#else
#define errno  (*(__errno()))
#endif

#endif

/** @}
 */
