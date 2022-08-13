/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcarm64
 * @{
 */
/** @file
 * @brief Syscall routine.
 */

#include <libc.h>

/** Syscall routine.
 *
 * Stores p1-p6, id to r0-r6 registers and calls <code>svc</code> instruction.
 * Returned value is read from r0 register.
 *
 * @param p1 Parameter 1.
 * @param p2 Parameter 2.
 * @param p3 Parameter 3.
 * @param p4 Parameter 4.
 * @param p5 Parameter 5.
 * @param p6 Parameter 6.
 * @param id Number of syscall.
 *
 * @return Syscall return value.
 */
sysarg_t __syscall(const sysarg_t p1, const sysarg_t p2, const sysarg_t p3,
    const sysarg_t p4, const sysarg_t p5, const sysarg_t p6,
    const syscall_t id)
{
	register sysarg_t __arm_reg_x0 asm("x0") = p1;
	register sysarg_t __arm_reg_x1 asm("x1") = p2;
	register sysarg_t __arm_reg_x2 asm("x2") = p3;
	register sysarg_t __arm_reg_x3 asm("x3") = p4;
	register sysarg_t __arm_reg_x4 asm("x4") = p5;
	register sysarg_t __arm_reg_x5 asm("x5") = p6;
	register sysarg_t __arm_reg_x6 asm("x6") = id;

	asm volatile (
	    "svc #0"
	    : "=r" (__arm_reg_x0)
	    : "r" (__arm_reg_x0),
	      "r" (__arm_reg_x1),
	      "r" (__arm_reg_x2),
	      "r" (__arm_reg_x3),
	      "r" (__arm_reg_x4),
	      "r" (__arm_reg_x5),
	      "r" (__arm_reg_x6)
	    :
	      /*
	       * Clobber memory too as some arguments might be actually
	       * pointers.
	       */
	      "memory"
	);

	return __arm_reg_x0;
}

/** @}
 */
