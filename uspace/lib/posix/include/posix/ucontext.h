/*
 * Copyright (c) 2011 Jiri Zarevucky
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
