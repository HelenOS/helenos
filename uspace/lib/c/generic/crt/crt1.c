/*
 * SPDX-FileCopyrightText: 2018 CZ.NIC, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
