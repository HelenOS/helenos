/*
 * Copyright (c) 2005 Martin Decky
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

#include <barrier.h>

#define COHERENCE_INVAL_MIN  4

/*
 * The IMB sequence used here is valid for all possible cache models
 * on uniprocessor. SMP might require a different sequence.
 * See PowerPC Programming Environment for 32-Bit Microprocessors,
 * chapter 5.1.5.2
 */

void smc_coherence(void *addr, size_t len)
{
	unsigned int i;

	for (i = 0; i < len; i += COHERENCE_INVAL_MIN)
		asm volatile (
		    "dcbst 0, %[addr]\n"
		    :: [addr] "r" (addr + i)
		);

	asm volatile ("sync" ::: "memory");

	for (i = 0; i < len; i += COHERENCE_INVAL_MIN)
		asm volatile (
		    "icbi 0, %[addr]\n"
		    :: [addr] "r" (addr + i)
		);

	asm volatile ("sync" ::: "memory");
	asm volatile ("isync" ::: "memory");
}
