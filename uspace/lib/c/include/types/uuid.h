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

#ifndef _LIBC_TYPES_UUID_H_
#define _LIBC_TYPES_UUID_H_

#include <stdint.h>

enum {
	uuid_bytes = 16
};

/** Universally Unique Identifier */
typedef struct {
	uint8_t b[uuid_bytes];
} uuid_t;

#endif

/** @}
 */
