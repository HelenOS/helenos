/*
 * SPDX-FileCopyrightText: 2011 Jiri Zarevucky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file Filename-matching.
 */

#ifndef POSIX_FNMATCH_H_
#define POSIX_FNMATCH_H_

#include <_bits/decls.h>

/* Error Values */
#define FNM_NOMATCH 1

/* Flags */
#define FNM_PATHNAME 1
#define FNM_PERIOD 2
#define FNM_NOESCAPE 4

/* GNU Extensions */
#define FNM_FILE_NAME FNM_PATHNAME
#define FNM_LEADING_DIR 8
#define FNM_CASEFOLD 16

__C_DECLS_BEGIN;

extern int fnmatch(const char *pattern, const char *string, int flags);

__C_DECLS_END;

#endif /* POSIX_FNMATCH_H_ */

/** @}
 */
