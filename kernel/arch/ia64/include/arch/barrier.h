/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_BARRIER_H_
#define KERN_ia64_BARRIER_H_

#define mf()	asm volatile ("mf\n" ::: "memory")

#define srlz_i()		\
	asm volatile (";; srlz.i ;;\n" ::: "memory")
#define srlz_d()		\
	asm volatile (";; srlz.d\n" ::: "memory")

#define fc_i(a)			\
	asm volatile ("fc.i %0\n" :: "r" ((a)) : "memory")
#define sync_i()		\
	asm volatile (";; sync.i\n" ::: "memory")

#endif

/** @}
 */
