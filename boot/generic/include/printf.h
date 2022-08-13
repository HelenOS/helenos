/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 */

#ifndef BOOT_PRINT_H_
#define BOOT_PRINT_H_

#include <stddef.h>
#include <stdarg.h>
#include <printf_verify.h>

#define EOF  (-1)

extern int puts(const char *);
extern int printf(const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(1, 2);
extern int vprintf(const char *, va_list);

#endif

/** @}
 */
