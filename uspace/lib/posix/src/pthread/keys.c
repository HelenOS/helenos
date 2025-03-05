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

#define DPRINTF(format, ...) ((void) 0)

static fibril_local bool fibril_initialized = false;
static atomic_ushort next_key = 1; // skip the key 'zero'

/*
 * For now, we just support maximum of 100 keys. This can be improved
 * in the future by implementing a dynamically growing array with
 * reallocations, but that will require more synchronization.
 */
#define PTHREAD_KEYS_MAX 100
static void (*destructors[PTHREAD_KEYS_MAX])(void *);

static fibril_local void *key_data[PTHREAD_KEYS_MAX];

void *pthread_getspecific(pthread_key_t key)
{
	// initialization is done in setspecific -> if not initialized, nothing was set yet
	if (!fibril_initialized)
		return NULL;

	assert(key < PTHREAD_KEYS_MAX);
	assert(key < next_key);
	assert(key > 0);

	return key_data[key];
}

static void pthread_key_on_fibril_exit(void)
{
	if (!fibril_initialized)
		return;

	for (unsigned i = 0; i < PTHREAD_KEYS_MAX; i++) {
		/*
		 * Note that this doesn't cause a race with pthread_key_create:
		 * if key `i` has not been assigned yet (it could be just being
		 * created), key_data[i] has never been assigned, therefore it
		 * is NULL, and the destructor is not checked at all.
		 */
		if (key_data[i] != NULL && destructors[i] != NULL)
			destructors[i](key_data[i]);
	}
}

int pthread_setspecific(pthread_key_t key, const void *data)
{
	if (!fibril_initialized) {
		DPRINTF("initializing pthread keys\n");
		errno_t res = fibril_add_exit_hook(pthread_key_on_fibril_exit);
		if (res != EOK)
			return res;

		for (unsigned i = 0; i < PTHREAD_KEYS_MAX; i++) {
			key_data[i] = NULL;
		}
		fibril_initialized = true;
	}
	assert(key < PTHREAD_KEYS_MAX);
	assert(key < next_key);
	assert(key > 0);

	key_data[key] = (void *) data;
	return EOK;
}

int pthread_key_delete(pthread_key_t key)
{
	/*
	 * FIXME: this can cause a data race if another fibrill concurrently
	 * runs on_fibril_exit. The obvious solution is to add a rwlock on
	 * the destructors array, which will be needed anyway if we want to
	 * support unlimited number of keys.
	 */
	destructors[key] = NULL;
	key_data[key] = NULL;

	// TODO: the key could also be reused
	return EOK;
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
	unsigned short k = atomic_fetch_add(&next_key, 1);
	if (k >= PTHREAD_KEYS_MAX) {
		atomic_store(&next_key, PTHREAD_KEYS_MAX + 1);
		return ELIMIT;
	}

	destructors[k] = destructor;
	*key = k;
	return EOK;
}

/** @}
 */
