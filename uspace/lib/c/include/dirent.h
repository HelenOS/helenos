/*
 * SPDX-FileCopyrightText: 2008 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_DIRENT_H_
#define _LIBC_DIRENT_H_

#include <_bits/decls.h>

__C_DECLS_BEGIN;

struct dirent {
	char d_name[256];
};

typedef struct __dirstream DIR;

extern DIR *opendir(const char *);
extern struct dirent *readdir(DIR *);
extern void rewinddir(DIR *);
extern int closedir(DIR *);

__C_DECLS_END;

#endif

/** @}
 */
