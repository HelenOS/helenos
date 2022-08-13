/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libddev
 * @{
 */
/** @file
 */

#ifndef _LIBDDEV_TYPES_DDEV_H_
#define _LIBDDEV_TYPES_DDEV_H_

#include <async.h>

/** Display server session */
typedef struct {
	/** Session with display device */
	async_sess_t *sess;
} ddev_t;

#endif

/** @}
 */
