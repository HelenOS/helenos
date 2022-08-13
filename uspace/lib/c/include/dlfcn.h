/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup rtld
 * @{
 */
/** @file
 * @brief UNIX-like dynamic linker interface.
 */

#ifndef _LIBC_DLFCN_H_
#define _LIBC_DLFCN_H_

#include <_bits/decls.h>

__C_DECLS_BEGIN;

void *dlopen(const char *, int);
void *dlsym(void *, const char *);

__C_DECLS_END;

#endif

/**
 * @}
 */
