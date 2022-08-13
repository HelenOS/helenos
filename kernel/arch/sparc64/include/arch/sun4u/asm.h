/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_sun4u_ASM_H_
#define KERN_sparc64_sun4u_ASM_H_

#include <trace.h>

/** Read Version Register.
 *
 * @return Value of VER register.
 *
 */
_NO_TRACE static inline uint64_t ver_read(void)
{
	uint64_t v;

	asm volatile (
	    "rdpr %%ver, %[v]\n"
	    : [v] "=r" (v)
	);

	return v;
}

extern uint64_t read_from_ag_g7(void);
extern void write_to_ag_g6(uint64_t);
extern void write_to_ag_g7(uint64_t);
extern void write_to_ig_g6(uint64_t);

#endif

/** @}
 */
