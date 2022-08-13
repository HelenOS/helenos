/*
 * SPDX-FileCopyrightText: 2011 Jiri Zarevucky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file Userspace context handling.
 */

#ifndef POSIX_UCONTEXT_H_
#define POSIX_UCONTEXT_H_

#include <sys/types.h>
#include <_bits/decls.h>

__C_DECLS_BEGIN;

typedef int sig_atomic_t;
typedef uint32_t sigset_t;
typedef struct mcontext {
	/* must not be empty to avoid compiler warnings (-pedantic) */
	int dummy;
} mcontext_t;

union sigval {
	int sival_int;
	void *sival_ptr;
};

struct sigevent {
	int sigev_notify; /* Notification type. */
	int sigev_signo; /* Signal number. */
	union sigval sigev_value; /* Signal value. */
	void (*sigev_notify_function)(union sigval); /* Notification function. */
	thread_attr_t *sigev_notify_attributes; /* Notification attributes. */
};

typedef struct {
	void *ss_sp;
	size_t ss_size;
	int ss_flags;
} stack_t;

typedef struct ucontext {
	struct ucontext *uc_link;
	sigset_t uc_sigmask;
	stack_t uc_stack;
	mcontext_t uc_mcontext;
} ucontext_t;

__C_DECLS_END;

#endif

/** @}
 */
