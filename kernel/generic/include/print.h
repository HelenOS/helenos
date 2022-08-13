/*
 * SPDX-FileCopyrightText: 2001-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_PRINT_H_
#define KERN_PRINT_H_

#include <stdarg.h>
#include <stddef.h>
#include <printf/verify.h>

#define EOF  (-1)

extern int puts(const char *s);
extern int printf(const char *fmt, ...)
    _HELENOS_PRINTF_ATTRIBUTE(1, 2);
extern int snprintf(char *str, size_t size, const char *fmt, ...)
    _HELENOS_PRINTF_ATTRIBUTE(3, 4);

extern int vprintf(const char *fmt, va_list ap);
extern int vsnprintf(char *str, size_t size, const char *fmt, va_list ap);

#endif

/** @}
 */
