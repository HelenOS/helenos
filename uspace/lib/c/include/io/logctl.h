/*
 * SPDX-FileCopyrightText: 2011-2012 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */

#ifndef _LIBC_IO_LOGCTL_H_
#define _LIBC_IO_LOGCTL_H_

#include <io/log.h>

extern errno_t logctl_set_default_level(log_level_t);
extern errno_t logctl_set_log_level(const char *, log_level_t);
extern errno_t logctl_set_root(void);

#endif

/** @}
 */
