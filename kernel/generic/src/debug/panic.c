/*
 * SPDX-FileCopyrightText: 2010 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_debug
 * @{
 */
/** @file
 */

#include <panic.h>
#include <stdio.h>
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

	printf("CURRENT=%p: ", CURRENT);
	if (CURRENT != NULL) {
		printf("pe=%" PRIuPTR " thread=%p task=%p cpu=%p as=%p"
		    " magic=%#" PRIx32 "\n", CURRENT->preemption,
		    CURRENT->thread, CURRENT->task, CURRENT->cpu, CURRENT->as, CURRENT->magic);

		if (CURRENT->thread != NULL)
			printf("thread=\"%s\"\n", CURRENT->thread->name);

		if (CURRENT->task != NULL)
			printf("task=\"%s\"\n", CURRENT->task->name);
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
