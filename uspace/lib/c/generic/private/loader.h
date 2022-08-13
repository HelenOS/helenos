/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_PRIVATE_LOADER_H_
#define _LIBC_PRIVATE_LOADER_H_

#include <loader/loader.h>

/** Abstraction of a loader connection */
struct loader {
	/** Session to the loader. */
	async_sess_t *sess;
};

#endif

/** @}
 */
