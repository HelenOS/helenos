/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcsparc64
 * @{
 */
/** @file
 */

#ifndef _LIBC_sparc64_STACK_H_
#define _LIBC_sparc64_STACK_H_

#define STACK_ITEM_SIZE			8

/** According to SPARC Compliance Definition, every stack frame is 16-byte aligned. */
#define STACK_ALIGNMENT			16

/**
 * 16-extended-word save area for %i[0-7] and %l[0-7] registers.
 */
#define STACK_WINDOW_SAVE_AREA_SIZE	(16 * STACK_ITEM_SIZE)

/*
 * Six extended words for first six arguments.
 */
#define STACK_ARG_SAVE_AREA_SIZE		(6 * STACK_ITEM_SIZE)

/**
 * By convention, the actual top of the stack is %sp + STACK_BIAS.
 */
#define STACK_BIAS            2047

#endif

/** @}
 */
