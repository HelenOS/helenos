/*
 * Copyright (c) 2025 Miroslav Cimerman
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

#include <stdbool.h>
#include <fibril_synch.h>

static FIBRIL_MUTEX_INITIALIZE(_atomic_mtx);

unsigned long long __atomic_load_8(const volatile void *mem0, int model)
{
	const volatile unsigned long long *mem = mem0;

	(void)model;

	unsigned long long ret;

	fibril_mutex_lock(&_atomic_mtx);
	ret = *mem;
	fibril_mutex_unlock(&_atomic_mtx);

	return ret;
}

void __atomic_store_8(volatile void *mem0, unsigned long long val, int model)
{
	volatile unsigned long long *mem = mem0;

	(void)model;

	fibril_mutex_lock(&_atomic_mtx);
	*mem = val;
	fibril_mutex_unlock(&_atomic_mtx);
}

bool __atomic_compare_exchange_1(volatile void *mem0, void *expected0,
    unsigned char desired, bool weak, int success, int failure)
{
	volatile unsigned char *mem = mem0;
	unsigned char *expected = expected0;
	unsigned char ov = *expected;

	(void)success;
	(void)failure;
	(void)weak;

	fibril_mutex_lock(&_atomic_mtx);

	unsigned char ret = *mem;

	if (ret == ov) {
		*mem = desired;
		fibril_mutex_unlock(&_atomic_mtx);
		return true;
	}

	*expected = ret;
	fibril_mutex_unlock(&_atomic_mtx);
	return false;
}
