/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup abi_generic
 * @{
 */
/** @file
 */

#ifndef _ABI_PROC_UARG_H_
#define _ABI_PROC_UARG_H_

#include <stddef.h>
#include <_bits/native.h>

typedef void uspace_thread_function_t(void *);

/** Structure passed to uinit kernel thread as argument. */
typedef struct uspace_arg {
	uspace_addr_t uspace_entry;
	uspace_addr_t uspace_stack;
	size_t uspace_stack_size;

	uspace_ptr_uspace_thread_function_t uspace_thread_function;
	uspace_addr_t uspace_thread_arg;

	uspace_ptr_struct_uspace_arg uspace_uarg;
} uspace_arg_t;

#endif

/** @}
 */
