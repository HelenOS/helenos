/*
 * Copyright (C) 2005 Jakub Jermar
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

#ifndef __ia64_ATOMIC_H__
#define __ia64_ATOMIC_H__

#include <arch/types.h>

typedef volatile __u64 atomic_t;

static inline atomic_t atomic_add(atomic_t *val, int imm)
{
	atomic_t v;

	
 	__asm__ volatile ("fetchadd8.rel %0 = %1, %2\n" : "=r" (v), "+m" (*val) : "i" (imm));
 
 	*val += imm;
	
	return v;
}

static inline void atomic_inc(atomic_t *val) { atomic_add(val, 1); }
static inline void atomic_dec(atomic_t *val) { atomic_add(val, -1); }


static inline atomic_t atomic_inc_pre(atomic_t *val) { return atomic_add(val, 1); }
static inline atomic_t atomic_dec_pre(atomic_t *val) { return atomic_add(val, -1); }


static inline atomic_t atomic_inc_post(atomic_t *val) { return atomic_add(val, 1)+1; }
static inline atomic_t atomic_dec_post(atomic_t *val) { return atomic_add(val, -1)-1; }




#endif
