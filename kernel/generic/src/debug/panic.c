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
#include <func.h>
#include <typedefs.h>
#include <mm/as.h>
#include <stdarg.h>
#include <interrupt.h>

void panic_common(panic_category_t cat, istate_t *istate, int access,
    uintptr_t address, const char *fmt, ...)
{
	va_list args;

	silent = false;

	printf("\nKERNEL PANIC ");
	if (CPU)
		printf("ON cpu%d ", CPU->id);
	printf("DUE TO ");

	va_start(args, fmt);
	if (cat == PANIC_ASSERT) {
		printf("A FAILED ASSERTION:\n");
		vprintf(fmt, args);
		printf("\n");
	} else if (cat == PANIC_BADTRAP) {
		printf("BAD TRAP %ld.\n", address);
	} else if (cat == PANIC_MEMTRAP) {
		printf("A BAD MEMORY ACCESS WHILE ");
		if (access == PF_ACCESS_READ)
			printf("LOADING FROM");
		else if (access == PF_ACCESS_WRITE)
			printf("STORING TO");
		else if (access == PF_ACCESS_EXEC)
			printf("BRANCHING TO");
		else
			printf("REFERENCING");
		printf(" ADDRESS %p.\n", address); 
	} else {
		printf("THE FOLLOWING REASON:\n");
		vprintf(fmt, args);
	}
	va_end(args);

	printf("\n");

	if (istate) {
		decode_istate(istate);
		printf("\n");
	}

	stack_trace();
	halt();
}

/** @}
 */
