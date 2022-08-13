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

#ifndef LIB_PERM_H_
#define LIB_PERM_H_

#include <task.h>

extern errno_t perm_grant(task_id_t, unsigned int);
extern errno_t perm_revoke(task_id_t, unsigned int);

#endif

/** @}
 */
