/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_SYSINFO_H_
#define _LIBC_SYSINFO_H_

#include <types/common.h>
#include <stddef.h>
#include <stdbool.h>
#include <abi/sysinfo.h>

extern char *sysinfo_get_keys(const char *, size_t *);
extern sysinfo_item_val_type_t sysinfo_get_val_type(const char *);
extern errno_t sysinfo_get_value(const char *, sysarg_t *);
extern void *sysinfo_get_data(const char *, size_t *);
extern void *sysinfo_get_property(const char *, const char *, size_t *);

#endif

/** @}
 */
