/*
 * SPDX-FileCopyrightText: 2009 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_CLIPBOARD_H_
#define _LIBC_CLIPBOARD_H_

#include <errno.h>

extern errno_t clipboard_put_str(const char *);
extern errno_t clipboard_get_str(char **);

#endif

/** @}
 */
