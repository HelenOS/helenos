/*
 * SPDX-FileCopyrightText: 2017 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */

#ifndef POSIX_DLFCN_H_
#define POSIX_DLFCN_H_

#include <libc/dlfcn.h>

#define RTLD_LAZY 1
#define RTLD_NOW 2
#define RTLD_GLOBAL 32
#define RTLD_LOCAL 0

__C_DECLS_BEGIN;

extern void *dlopen(const char *, int);
extern void *dlsym(void *, const char *);
extern int dlclose(void *);
extern char *dlerror(void);

__C_DECLS_END;

#endif

/**
 * @}
 */
