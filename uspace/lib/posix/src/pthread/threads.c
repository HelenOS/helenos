/*
 * SPDX-FileCopyrightText: 2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file Pthread: thread management.
 */

#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <fibril.h>
#include "../internal/common.h"

pthread_t pthread_self(void)
{
	return (pthread_t) fibril_get_id();
}

int pthread_equal(pthread_t thread1, pthread_t thread2)
{
	return thread1 == thread2;
}

int pthread_create(pthread_t *thread_id, const pthread_attr_t *attributes,
    void *(*start_routine)(void *), void *arg)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_join(pthread_t thread, void **ret_val)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_detach(pthread_t thread)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_attr_init(pthread_attr_t *attr)
{
	not_implemented();
	return ENOTSUP;
}

int pthread_attr_destroy(pthread_attr_t *attr)
{
	not_implemented();
	return ENOTSUP;
}

/** @}
 */
