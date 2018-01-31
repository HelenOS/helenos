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
/** @file Pthread: mutexes.
 */

#include "posix/pthread.h"
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
