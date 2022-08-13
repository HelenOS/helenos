/*
 * SPDX-FileCopyrightText: 2009 Lukas Mejdrech
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_ARG_PARSE_H_
#define _LIBC_ARG_PARSE_H_

#include <errno.h>

typedef errno_t (*arg_parser)(const char *, int *);

extern int arg_parse_short_long(const char *, const char *, const char *);
extern errno_t arg_parse_int(int, char **, int *, int *, int);
extern errno_t arg_parse_name_int(int, char **, int *, int *, int, arg_parser);
extern errno_t arg_parse_string(int, char **, int *, char **, int);

#endif

/** @}
 */
