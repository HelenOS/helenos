/*
 * Copyright (c) 2005 Ondrej Palkovsky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
