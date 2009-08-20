/*
 * Copyright (c) 2008 Jiri Svoboda
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

/** @addtogroup rtld
 * @brief	Test dynamic linking capabilities.
 * @{
 */ 
/**
 * @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

static void kputint(unsigned i)
{
//	unsigned dummy;
//	asm volatile (
//		"movl $30, %%eax;"
//		"int $0x30"
//		: "=d" (dummy) /* output - %edx clobbered */
//		: "d" (i) /* input */
//		: "%eax","%ecx" /* all scratch registers clobbered */
//	);
/*
	asm volatile (
		"mr %%r3, %0\n"
		"li %%r9, 32\n"
		"sc\n"
		:
		: "r" (i)
		: "%r3","%r9"
	);
*/
}

typedef int (*fptr_t)(void);

int main(int argc, char *argv[])
{
	void *a;
	void *s;
	fptr_t fun;
	int i;

	char *lib_name;
	char *sym_name;

//	kputint(-1);
	printf("Hello from dltest!\n");

	lib_name = "libtest.so.0";
	sym_name = "test_func";

	a = dlopen(lib_name, 0);
	if (a != NULL) {
		s = dlsym(a, sym_name);
		printf("symbol '%s' = 0x%lx\n", sym_name, (long) s);
	} else {
		printf("failed to dlopen() library '%s'\n", lib_name);
	}

	printf("Run dynamically-resolved function '%s'...\n", sym_name);
	fun = (fptr_t) s;
	i = (*fun)();
	printf("Done. (returned 0x%x)\n", i);
	
	return 0;
}

/** @}
 */

