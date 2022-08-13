/*
 * SPDX-FileCopyrightText: 2007 Pavel Jancik
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcarm32
 * @{
 */
/** @file
 *  @brief Syscall routine.
 */

#include <libc.h>

/** Syscall routine.
 *
 *  Stores p1-p4, id to r0-r4 registers and calls <code>swi</code>
 *  instruction. Returned value is read from r0 register.
 *
 *  @param p1 Parameter 1.
 *  @param p2 Parameter 2.
 *  @param p3 Parameter 3.
 *  @param p4 Parameter 4.
 *  @param id Number of syscall.
 *
 *  @return Syscall return value.
 */
sysarg_t __syscall(const sysarg_t p1, const sysarg_t p2, const sysarg_t p3,
    const sysarg_t p4, const sysarg_t p5, const sysarg_t p6, const syscall_t id)
{
	register sysarg_t __arm_reg_r0 asm("r0") = p1;
	register sysarg_t __arm_reg_r1 asm("r1") = p2;
	register sysarg_t __arm_reg_r2 asm("r2") = p3;
	register sysarg_t __arm_reg_r3 asm("r3") = p4;
	register sysarg_t __arm_reg_r4 asm("r4") = p5;
	register sysarg_t __arm_reg_r5 asm("r5") = p6;
	register sysarg_t __arm_reg_r6 asm("r6") = id;

	asm volatile (
	    "swi 0"
	    : "=r" (__arm_reg_r0)
	    : "r" (__arm_reg_r0),
	      "r" (__arm_reg_r1),
	      "r" (__arm_reg_r2),
	      "r" (__arm_reg_r3),
	      "r" (__arm_reg_r4),
	      "r" (__arm_reg_r5),
	      "r" (__arm_reg_r6)
	    :
	      /*
	       * Clobber memory too as some arguments might be
	       * actually pointers.
	       */
	      "memory"
	);

	return __arm_reg_r0;
}

/** @}
 */
