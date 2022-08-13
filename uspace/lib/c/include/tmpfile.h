/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LIBC_TMPFILE_H_
#define _LIBC_TMPFILE_H_

#include <stdbool.h>

extern int __tmpfile_templ(char *, bool);
extern int __tmpfile(void);
extern char *__tmpnam(char *);

#endif
