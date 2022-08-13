/*
 * SPDX-FileCopyrightText: 2016 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_CONFIG_H_
#define _LIBC_CONFIG_H_

#include <stdbool.h>

extern bool config_key_exists(const char *);
extern char *config_get_value(const char *);

#endif

/** @}
 */
