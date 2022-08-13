/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_PRIVATE_STDLIB_H_
#define _LIBC_PRIVATE_STDLIB_H_

#include <adt/list.h>

/** Exit handler list entry */
typedef struct {
	/** Link to exit handler list */
	link_t llist;
	/** Exit handler function */
	void (*func)(void);
} __exit_handler_t;

#endif

/** @}
 */
