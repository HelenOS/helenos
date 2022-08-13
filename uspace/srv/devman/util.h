/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup devman
 * @{
 */

#ifndef DEVMAN_UTIL_H_
#define DEVMAN_UTIL_H_

#include <ctype.h>
#include <stdlib.h>
#include <str.h>

extern char *get_abs_path(const char *, const char *, const char *);
extern char *get_path_elem_end(char *);

extern bool skip_spaces(char **);
extern void skip_line(char **);
extern size_t get_nonspace_len(const char *);
extern void replace_char(char *, char, char);

#endif

/** @}
 */
