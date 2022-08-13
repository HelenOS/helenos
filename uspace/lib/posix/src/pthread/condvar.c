/*
 * SPDX-FileCopyrightText: 2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file Pthread: condition variables.
 */

#include <pthread.h>
#include <errno.h>
#include "../internal/common.h"

int pthread_cond_init(pthread_cond_t *restrict condvar,
    const pthread_condattr_t *restrict attr)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_cond_destroy(pthread_cond_t *condvar)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_cond_broadcast(pthread_cond_t *condvar)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_cond_signal(pthread_cond_t *condvar)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_cond_timedwait(pthread_cond_t *restrict condvar,
    pthread_mutex_t *restrict mutex, const struct timespec *restrict timeout)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_cond_wait(pthread_cond_t *restrict condvar,
    pthread_mutex_t *restrict mutex)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_condattr_init(pthread_condattr_t *attr)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_condattr_destroy(pthread_condattr_t *attr)
{
	not_implemented();
	return ENOTSUP;
}

/** @}
 */
