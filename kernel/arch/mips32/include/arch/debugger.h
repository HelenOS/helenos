/*
 * SPDX-FileCopyrightText: 2005 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32_debug
 * @{
 */
/** @file
 */

#ifndef KERN_mips32_DEBUGGER_H_
#define KERN_mips32_DEBUGGER_H_

#include <arch/exception.h>
#include <typedefs.h>
#include <stdbool.h>

#define BKPOINTS_MAX 10

/** Breakpoint was shot */
#define BKPOINT_INPROG  (1 << 0)

/** One-time breakpoint, mandatory for j/b instructions */
#define BKPOINT_ONESHOT  (1 << 1)

/**
 * Breakpoint is set on the next instruction, so that it
 * could be reinstalled on the previous one
 */
#define BKPOINT_REINST  (1 << 2)

/** Call a predefined function */
#define BKPOINT_FUNCCALL  (1 << 3)

typedef struct  {
	uintptr_t address;         /**< Breakpoint address */
	sysarg_t instruction;      /**< Original instruction */
	sysarg_t nextinstruction;  /**< Original instruction following break */
	unsigned int flags;        /**< Flags regarding breakpoint */
	size_t counter;
	void (*bkfunc)(void *, istate_t *);
} bpinfo_t;

extern bpinfo_t breakpoints[BKPOINTS_MAX];

extern bool is_jump(sysarg_t);

extern void debugger_init(void);
extern void debugger_bpoint(istate_t *);

#endif

/** @}
 */
