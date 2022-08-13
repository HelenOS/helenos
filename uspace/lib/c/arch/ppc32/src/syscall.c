/*
 * SPDX-FileCopyrightText: 2006 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcppc32 ppc32
 * @brief ppc32 architecture dependent parts of libc
 * @ingroup lc
 * @{
 */
/** @file
 */

#include <libc.h>

sysarg_t __syscall(const sysarg_t p1, const sysarg_t p2, const sysarg_t p3,
    const sysarg_t p4, const sysarg_t p5, const sysarg_t p6, const syscall_t id)
{
	register sysarg_t __ppc32_reg_r3 asm("3") = p1;
	register sysarg_t __ppc32_reg_r4 asm("4") = p2;
	register sysarg_t __ppc32_reg_r5 asm("5") = p3;
	register sysarg_t __ppc32_reg_r6 asm("6") = p4;
	register sysarg_t __ppc32_reg_r7 asm("7") = p5;
	register sysarg_t __ppc32_reg_r8 asm("8") = p6;
	register sysarg_t __ppc32_reg_r9 asm("9") = id;

	asm volatile (
	    "sc\n"
	    : "=r" (__ppc32_reg_r3)
	    : "r" (__ppc32_reg_r3),
	      "r" (__ppc32_reg_r4),
	      "r" (__ppc32_reg_r5),
	      "r" (__ppc32_reg_r6),
	      "r" (__ppc32_reg_r7),
	      "r" (__ppc32_reg_r8),
	      "r" (__ppc32_reg_r9)
	    :
	      /*
	       * Clobber memory too as some arguments might be
	       * actually pointers.
	       */
	      "memory"
	);

	return __ppc32_reg_r3;
}

/** @}
 */
