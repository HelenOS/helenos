/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_STACK_H_
#define KERN_sparc64_STACK_H_

#include <config.h>

#define MEM_STACK_SIZE	STACK_SIZE

#define STACK_ITEM_SIZE			8

/** According to SPARC Compliance Definition, every stack frame is 16-byte aligned. */
#define STACK_ALIGNMENT			16

/**
 * 16-extended-word save area for %i[0-7] and %l[0-7] registers.
 */
#define STACK_WINDOW_SAVE_AREA_SIZE	(16 * STACK_ITEM_SIZE)

/**
 * Six extended words for first six arguments.
 */
#define STACK_ARG_SAVE_AREA_SIZE	(6 * STACK_ITEM_SIZE)

/**
 * By convention, the actual top of the stack is %sp + STACK_BIAS.
 */
#define STACK_BIAS            2047

/*
 * Offsets of arguments on stack.
 */
#define STACK_ARG0			0
#define STACK_ARG1			8
#define STACK_ARG2			16
#define STACK_ARG3			24
#define STACK_ARG4			32
#define STACK_ARG5			40
#define STACK_ARG6			48

#endif

/** @}
 */
