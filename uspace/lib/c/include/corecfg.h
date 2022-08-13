/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_CORECFG_H_
#define _LIBC_CORECFG_H_

#include <errno.h>
#include <stdbool.h>

extern errno_t corecfg_init(void);
extern errno_t corecfg_set_enable(bool);
extern errno_t corecfg_get_enable(bool *);

#endif

/** @}
 */
