/*
 * SPDX-FileCopyrightText: 2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file Pthread: keys and thread-specific storage.
 */

#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include "../internal/common.h"

void *pthread_getspecific(pthread_key_t key)
{
	not_implemented();
	return NULL;
}

int pthread_setspecific(pthread_key_t key, const void *data)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_key_delete(pthread_key_t key)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
	not_implemented();
	return ENOTSUP;
}

/** @}
 */
