/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup trace
 * @{
 */
/** @file
 */

#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#include <stdbool.h>
#include <stddef.h>

#include "trace.h"

typedef struct {
	const char *name;
	int n_args;
	val_type_t rv_type;
} sc_desc_t;

extern const sc_desc_t syscall_desc[];
extern const size_t syscall_desc_len;

static inline bool syscall_desc_defined(unsigned sc_id)
{
	return (sc_id < syscall_desc_len && syscall_desc[sc_id].name != NULL);
}

#endif

/** @}
 */
