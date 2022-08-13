/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_abs32le
 * @{
 */
/** @file
 */

#ifndef KERN_abs32le_CONTEXT_H_
#define KERN_abs32le_CONTEXT_H_

#define STACK_ITEM_SIZE  4
#define SP_DELTA         0

#define context_set(ctx, pc, stack, size) \
	context_set_generic(ctx, pc, stack, size)

/*
 * On real hardware this stores the registers which
 * need to be preserved across function calls.
 */
typedef struct {
	uintptr_t sp;
	uintptr_t pc;
	ipl_t ipl;
} context_t;

#endif

/** @}
 */
