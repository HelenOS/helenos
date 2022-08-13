/*
 * SPDX-FileCopyrightText: 2009 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <errno.h>
#include <fibril.h>

static fibril_local errno_t fibril_errno;

errno_t *__errno(void)
{
	return &fibril_errno;
}

/** @}
 */
