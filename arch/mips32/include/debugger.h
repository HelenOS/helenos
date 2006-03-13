/*
 * Copyright (C) 2005 Ondrej Palkovsky
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

#ifndef _mips32_DEBUGGER_H_
#define _mips32_DEBUGGER_H_

#include <typedefs.h>
#include <arch/exception.h>
#include <arch/types.h>

#define BKPOINTS_MAX 10

#define BKPOINT_INPROG   (1 << 0)   /**< Breakpoint was shot */
#define BKPOINT_ONESHOT  (1 << 1)   /**< One-time breakpoint,mandatory for j/b
				         instructions */
#define BKPOINT_REINST   (1 << 2)   /**< Breakpoint is set on the next 
				         instruction, so that it could be
					 reinstalled on the previous one */
#define BKPOINT_FUNCCALL (1 << 3)   /**< Call a predefined function */

typedef struct  {
	__address address;      /**< Breakpoint address */
	__native instruction; /**< Original instruction */
	__native nextinstruction;  /**< Original instruction following break */
	int flags;        /**< Flags regarding breakpoint */
	count_t counter;
	void (*bkfunc)(void *b, istate_t *istate);
} bpinfo_t;

extern void debugger_init(void);
void debugger_bpoint(istate_t *istate);

extern bpinfo_t breakpoints[BKPOINTS_MAX];

#endif
