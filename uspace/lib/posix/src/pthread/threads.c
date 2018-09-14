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
