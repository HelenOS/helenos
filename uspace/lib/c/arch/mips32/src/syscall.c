/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcmips32
 * @{
 */
/** @file
 * @ingroup libcmips32
 */

#include <libc.h>

sysarg_t __syscall(const sysarg_t p1, const sysarg_t p2, const sysarg_t p3,
    const sysarg_t p4, const sysarg_t p5, const sysarg_t p6, const syscall_t id)
{
	register sysarg_t __mips_reg_a0 asm("$4") = p1;
	register sysarg_t __mips_reg_a1 asm("$5") = p2;
	register sysarg_t __mips_reg_a2 asm("$6") = p3;
	register sysarg_t __mips_reg_a3 asm("$7") = p4;
	register sysarg_t __mips_reg_t0 asm("$8") = p5;
	register sysarg_t __mips_reg_t1 asm("$9") = p6;
	register sysarg_t __mips_reg_v0 asm("$2") = id;

	asm volatile (
	    "syscall\n"
	    : "=r" (__mips_reg_v0)
	    : "r" (__mips_reg_a0),
	      "r" (__mips_reg_a1),
	      "r" (__mips_reg_a2),
	      "r" (__mips_reg_a3),
	      "r" (__mips_reg_t0),
	      "r" (__mips_reg_t1),
	      "r" (__mips_reg_v0)
	    :
	      /*
	       * We are a function call, although C
	       * does not know it.
	       */
	      "%ra",
	      /*
	       * Clobber memory too as some arguments might be
	       * actually pointers.
	       */
	      "memory"
	);

	return __mips_reg_v0;
}

/** @}
 */
