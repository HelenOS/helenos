/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_UUID_H_
#define _LIBC_UUID_H_

#include <stdint.h>
#include <types/uuid.h>
#include <stdbool.h>

extern errno_t uuid_generate(uuid_t *);
extern void uuid_encode(uuid_t *, uint8_t *);
extern void uuid_decode(uint8_t *, uuid_t *);
extern errno_t uuid_parse(const char *, uuid_t *, const char **);
extern errno_t uuid_format(uuid_t *, char **, bool);

#endif

/** @}
 */
