/*
 * SPDX-FileCopyrightText: 2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file Pthread: mutexes.
 */

#include <pthread.h>
#include <errno.h>
#include "../internal/common.h"

int pthread_mutex_init(pthread_mutex_t *restrict mutex,
    const pthread_mutexattr_t *restrict attr)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t *restrict attr,
    int *restrict type)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
	not_implemented();
	return ENOTSUP;
}

/** @}
 */
