/*
 * Copyright (c) 2025 Matej Volf
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

#include <fibril.h>
#include <posix/pthread.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(pthread_keys);

pthread_key_t key;
int destructors_executed;

static void destructor(void *_data)
{
	destructors_executed++;
}

static errno_t simple_fibril(void *_arg)
{
	PCUT_ASSERT_INT_EQUALS(0, pthread_setspecific(key, (void *) 0x0d9e));
	PCUT_ASSERT_PTR_EQUALS((void *) 0x0d9e, pthread_getspecific(key));

	for (int i = 0; i < 10; i++) {
		fibril_yield();
	}

	return EOK;
}

PCUT_TEST(pthread_keys_basic)
{
	destructors_executed = 0;
	PCUT_ASSERT_INT_EQUALS(0, pthread_key_create(&key, destructor));
	PCUT_ASSERT_PTR_EQUALS(NULL, pthread_getspecific(key));

	PCUT_ASSERT_INT_EQUALS(0, pthread_setspecific(key, (void *) 0x42));
	PCUT_ASSERT_PTR_EQUALS((void *) 0x42, pthread_getspecific(key));

	fid_t other = fibril_create(simple_fibril, NULL);
	fibril_start(other);

	for (int i = 0; i < 5; i++) {
		fibril_yield();
	}

	PCUT_ASSERT_INT_EQUALS(0, destructors_executed);
	PCUT_ASSERT_PTR_EQUALS((void *) 0x42, pthread_getspecific(key));

	for (int i = 0; i < 10; i++) {
		fibril_yield();
	}

	PCUT_ASSERT_INT_EQUALS(1, destructors_executed);
	PCUT_ASSERT_PTR_EQUALS((void *) 0x42, pthread_getspecific(key));
}

PCUT_EXPORT(pthread_keys);
