/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 */

#ifndef BOOT_MACROS_H_
#define BOOT_MACROS_H_

#define min(a, b)  ((a) < (b) ? (a) : (b))

#define isdigit(d)  (((d) >= '0') && ((d) <= '9'))

#define STRING(arg)      STRING_ARG(arg)
#define STRING_ARG(arg)  #arg

#endif

/** @}
 */
