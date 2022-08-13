/*
 * SPDX-FileCopyrightText: 2011 Jiri Zarevucky
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file Character classification.
 */

#ifndef POSIX_CTYPE_H_
#define POSIX_CTYPE_H_

#include <libc/ctype.h>

__C_DECLS_BEGIN;

/* Obsolete Functions and Macros */
extern int isascii(int c);
extern int toascii(int c);

#define _tolower(c) ((c) - 'A' + 'a')
#define _toupper(c) ((c) - 'a' + 'A')

__C_DECLS_END;

#endif /* POSIX_CTYPE_H_ */

/** @}
 */
