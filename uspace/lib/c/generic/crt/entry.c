/*
 * Copyright (c) 2018 CZ.NIC, z.s.p.o.
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

#include "../private/libc.h"

/*
 * We shouldn't be accessing these symbols directly from libc,
 * since it would create unwanted relocations in shared library
 * code. Instead, we refer to them here, in a file that's always
 * statically linked to the main executable, and write their values
 * to a special structure that resides in libc.
 */

extern init_array_entry_t __preinit_array_start[] __attribute__((weak));
extern init_array_entry_t __preinit_array_end[] __attribute__((weak));
extern init_array_entry_t __init_array_start[] __attribute__((weak));
extern init_array_entry_t __init_array_end[] __attribute__((weak));
extern fini_array_entry_t __fini_array_start[] __attribute__((weak));
extern fini_array_entry_t __fini_array_end[] __attribute__((weak));
extern int main(int, char *[]);
extern unsigned char __executable_start[];
extern unsigned char _end[];

/* __c_start is only called from _start in assembly. */
void __c_start(void *);

void __c_start(void *pcb)
{
	__progsymbols = (progsymbols_t) {
		.main = main,
		.elfstart = __executable_start,
		.end = _end,
		.preinit_array = __preinit_array_start,
		.preinit_array_len = __preinit_array_end - __preinit_array_start,
		.init_array = __init_array_start,
		.init_array_len = __init_array_end - __init_array_start,
		.fini_array = __fini_array_start,
		.fini_array_len = __fini_array_end - __fini_array_start,
	};

	__libc_main(pcb);
}
