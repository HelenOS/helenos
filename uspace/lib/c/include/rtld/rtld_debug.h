/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_RTLD_RTLD_DEBUG_H_
#define _LIBC_RTLD_RTLD_DEBUG_H_

#include <stdio.h>

/* Define to enable debugging mode. */
#undef RTLD_DEBUG

#ifdef RTLD_DEBUG
#define DPRINTF(format, ...) printf(format, ##__VA_ARGS__)
#else
#define DPRINTF(format, ...) if (0) printf(format, ##__VA_ARGS__)
#endif

#endif

/** @}
 */
