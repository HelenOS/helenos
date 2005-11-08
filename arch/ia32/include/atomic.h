/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#ifndef __ia32_ATOMIC_H__
#define __ia32_ATOMIC_H__

#include <arch/types.h>

static inline void atomic_inc(volatile int *val) {
#ifdef CONFIG_SMP
	__asm__ volatile ("lock incl %0\n" : "=m" (*val));
#else
	__asm__ volatile ("incl %0\n" : "=m" (*val));
#endif /* CONFIG_SMP */
}

static inline void atomic_dec(volatile int *val) {
#ifdef CONFIG_SMP
	__asm__ volatile ("lock decl %0\n" : "=m" (*val));
#else
	__asm__ volatile ("decl %0\n" : "=m" (*val));
#endif /* CONFIG_SMP */
}

static inline int test_and_set(volatile int *val) {
	int v;
	
	__asm__ volatile (
		"movl $1, %0\n"
		"xchgl %0, %1\n"
		: "=r" (v),"=m" (*val)
	);
	
	return v;
}


extern void spinlock_arch(volatile int *val);

#endif
