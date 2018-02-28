/*
 * Copyright (c) 2010 Jakub Jermar
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

/** @addtogroup genericdebug
 * @{
 */
/** @file
 */

#include <panic.h>
#include <print.h>
#include <stacktrace.h>
#include <halt.h>
#include <typedefs.h>
#include <mm/as.h>
#include <stdarg.h>
#include <interrupt.h>

#define BANNER_LEFT   "######>"
#define BANNER_RIGHT  "<######"

void panic_common(panic_category_t cat, istate_t *istate, int access,
    uintptr_t address, const char *fmt, ...)
{
	console_override = true;

	printf("\n%s Kernel panic ", BANNER_LEFT);
	if (CPU)
		printf("on cpu%u ", CPU->id);
	printf("due to ");

	va_list args;
	va_start(args, fmt);
	if (cat == PANIC_ASSERT) {
		printf("a failed assertion: %s\n", BANNER_RIGHT);
		vprintf(fmt, args);
		printf("\n");
	} else if (cat == PANIC_BADTRAP) {
		printf("bad trap %" PRIuPTR ". %s\n", address,
		    BANNER_RIGHT);
		if (fmt) {
			vprintf(fmt, args);
			printf("\n");
		}
	} else if (cat == PANIC_MEMTRAP) {
		printf("a bad memory access while ");
		if (access == PF_ACCESS_READ)
			printf("loading from");
		else if (access == PF_ACCESS_WRITE)
			printf("storing to");
		else if (access == PF_ACCESS_EXEC)
			printf("branching to");
		else
			printf("referencing");
		printf(" address %p. %s\n", (void *) address,
		    BANNER_RIGHT);
		if (fmt) {
			vprintf(fmt, args);
			printf("\n");
		}
	} else {
		printf("the following reason: %s\n",
		    BANNER_RIGHT);
		vprintf(fmt, args);
		printf("\n");
	}
	va_end(args);

	printf("\n");

	printf("THE=%p: ", THE);
	if (THE != NULL) {
		printf("pe=%" PRIuPTR " thread=%p task=%p cpu=%p as=%p"
		    " magic=%#" PRIx32 "\n", THE->preemption,
		    THE->thread, THE->task, THE->cpu, THE->as, THE->magic);

		if (THE->thread != NULL)
			printf("thread=\"%s\"\n", THE->thread->name);

		if (THE->task != NULL)
			printf("task=\"%s\"\n", THE->task->name);
	} else
		printf("invalid\n");

	if (istate) {
		istate_decode(istate);
		printf("\n");
	}

	stack_trace();
	halt();
}

/** @}
 */
