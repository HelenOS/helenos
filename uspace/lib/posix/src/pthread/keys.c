/*
 * Copyright (c) 2013 Vojtech Horky
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

/** @addtogroup libposix
 * @{
 */
/** @file Pthread: keys and thread-specific storage.
 */

#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <fibril.h>
#include <stdatomic.h>
#include "../internal/common.h"

#include <stdio.h>
#define DPRINTF(format, ...) ((void) 0);

static atomic_ushort next_key = 1; // skip the key 'zero'

/*
 * For now, we just support maximum of 100 keys. This can be improved
 * in the future by implementing a dynamically growing array with
 * reallocations, but that will require more synchronization.
 */
#define PTHREAD_KEYS_MAX 100

static fibril_local void *key_data[PTHREAD_KEYS_MAX];

void *pthread_getspecific(pthread_key_t key)
{
	assert(key < PTHREAD_KEYS_MAX);
	assert(key < next_key);
	assert(key > 0);

	DPRINTF("pthread_getspecific(%d) = %p\n", key, key_data[key]);
	return key_data[key];
}

int pthread_setspecific(pthread_key_t key, const void *data)
{
	DPRINTF("pthread_setspecific(%d, %p)\n", key, data);
	assert(key < PTHREAD_KEYS_MAX);
	assert(key < next_key);
	assert(key > 0);

	key_data[key] = (void *) data;
	return EOK;
}

int pthread_key_delete(pthread_key_t key)
{
	/* see https://github.com/HelenOS/helenos/pull/245#issuecomment-2706795848 */
	not_implemented();
	return EOK;
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
	unsigned short k = atomic_fetch_add(&next_key, 1);
	DPRINTF("pthread_key_create(%p, %p) = %d\n", key, destructor, k);
	if (k >= PTHREAD_KEYS_MAX) {
		atomic_store(&next_key, PTHREAD_KEYS_MAX + 1);
		return ELIMIT;
	}
	if (destructor != NULL) {
		/* Inlined not_implemented() macro to add custom message */
		static int __not_implemented_counter = 0;
		if (__not_implemented_counter == 0) {
			fprintf(stderr, "pthread_key_create: destructors not supported\n");
		}
		__not_implemented_counter++;
	}

	*key = k;
	return EOK;
}

/** @}
 */
