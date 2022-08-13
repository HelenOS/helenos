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

#ifndef _LIBC_TYPES_TASK_H_
#define _LIBC_TYPES_TASK_H_

typedef enum {
	TASK_EXIT_NORMAL,
	TASK_EXIT_UNEXPECTED
} task_exit_t;

#endif

/** @}
 */
