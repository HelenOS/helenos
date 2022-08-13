/*
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBC_CTYPE_H_
#define _LIBC_CTYPE_H_

#include <_bits/decls.h>

__C_DECLS_BEGIN;
int islower(int);
int isupper(int);
int isalpha(int);
int isdigit(int);
int isalnum(int);
int isblank(int);
int iscntrl(int);
int isprint(int);
int isgraph(int);
int isspace(int);
int ispunct(int);
int isxdigit(int);
int tolower(int);
int toupper(int);
__C_DECLS_END;

#endif
