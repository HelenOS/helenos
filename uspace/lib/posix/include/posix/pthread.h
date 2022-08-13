/*
 * SPDX-FileCopyrightText: 2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
#ifndef POSIX_PTHREAD_H_
#define POSIX_PTHREAD_H_

#include <time.h>
#include <_bits/decls.h>

#define PTHREAD_MUTEX_RECURSIVE 1

#define PTHREAD_MUTEX_INITIALIZER { 0 }

#define PTHREAD_COND_INITIALIZER { 0 }

__C_DECLS_BEGIN;

typedef void *pthread_t;

typedef struct {
	int dummy;
} pthread_attr_t;

typedef int pthread_key_t;

typedef struct pthread_mutex {
	int dummy;
} pthread_mutex_t;

typedef struct {
	int dummy;
} pthread_mutexattr_t;

typedef struct {
	int dummy;
} pthread_condattr_t;

typedef struct {
	int dummy;
} pthread_cond_t;

extern pthread_t pthread_self(void);
extern int pthread_equal(pthread_t, pthread_t);
extern int pthread_create(pthread_t *, const pthread_attr_t *,
    void *(*)(void *), void *);
extern int pthread_join(pthread_t, void **);
extern int pthread_detach(pthread_t);

extern int pthread_attr_init(pthread_attr_t *);
extern int pthread_attr_destroy(pthread_attr_t *);

extern int pthread_mutex_init(pthread_mutex_t *__restrict__,
    const pthread_mutexattr_t *__restrict__);
extern int pthread_mutex_destroy(pthread_mutex_t *);
extern int pthread_mutex_lock(pthread_mutex_t *);
extern int pthread_mutex_trylock(pthread_mutex_t *);
extern int pthread_mutex_unlock(pthread_mutex_t *);

extern int pthread_mutexattr_init(pthread_mutexattr_t *);
extern int pthread_mutexattr_destroy(pthread_mutexattr_t *);
extern int pthread_mutexattr_gettype(const pthread_mutexattr_t *__restrict__,
    int *__restrict__);
extern int pthread_mutexattr_settype(pthread_mutexattr_t *, int);

extern int pthread_cond_init(pthread_cond_t *__restrict__,
    const pthread_condattr_t *__restrict__);
extern int pthread_cond_destroy(pthread_cond_t *);
extern int pthread_cond_broadcast(pthread_cond_t *);
extern int pthread_cond_signal(pthread_cond_t *);
extern int pthread_cond_timedwait(pthread_cond_t *__restrict__,
    pthread_mutex_t *__restrict__, const struct timespec *__restrict__);
extern int pthread_cond_wait(pthread_cond_t *__restrict__,
    pthread_mutex_t *__restrict__);

extern int pthread_condattr_destroy(pthread_condattr_t *);
extern int pthread_condattr_init(pthread_condattr_t *);

extern void *pthread_getspecific(pthread_key_t);
extern int pthread_setspecific(pthread_key_t, const void *);
extern int pthread_key_delete(pthread_key_t);
extern int pthread_key_create(pthread_key_t *, void (*)(void *));

__C_DECLS_END;

#endif

/** @}
 */
