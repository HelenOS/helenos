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
/** @file
 */

#ifndef POSIX_SIGNAL_H_
#define POSIX_SIGNAL_H_

#include "libc/errno.h"
#include "sys/types.h"

/* HelenOS doesn't have signals, so calls to functions of this header
 * are just replaced with their respective failure return value.
 *
 * Other macros and constants are here just to satisfy the symbol resolver
 * and have no practical value whatsoever, until HelenOS implements some
 * equivalent of signals. Maybe something neat based on IPC will be devised
 * in the future?
 */

#undef SIG_DFL
#define SIG_DFL ((void (*)(int)) 0)
#undef SIG_ERR
#define SIG_ERR ((void (*)(int)) 0)
#undef SIG_HOLD
#define SIG_HOLD ((void (*)(int)) 0)
#undef SIG_IGN
#define SIG_IGN ((void (*)(int)) 0)

#define signal(sig,func) (errno = ENOTSUP, SIG_ERR)
#define raise(sig) ((int) -1)
#define kill(pid,sig) (errno = ENOTSUP, (int) -1)

typedef int posix_sig_atomic_t;
typedef int posix_sigset_t;
typedef struct posix_mcontext {
} posix_mcontext_t;

union posix_sigval {
	int sival_int;
	void *sival_ptr;
};

struct posix_sigevent {
	int sigev_notify; /* Notification type. */
	int sigev_signo; /* Signal number. */
	union posix_sigval sigev_value; /* Signal value. */
	void (*sigev_notify_function)(union sigval); /* Notification function. */
	posix_thread_attr_t *sigev_notify_attributes; /* Notification attributes. */
};

typedef struct {
	int si_signo;
	int si_code;

	int si_errno;

	pid_t si_pid;
	uid_t si_uid;
	void *si_addr;
	int si_status;

	long si_band;

	union posix_sigval si_value;
} posix_siginfo_t;

struct posix_sigaction {
	void (*sa_handler)(int);
	posix_sigset_t sa_mask;
	int sa_flags;
	void (*sa_sigaction)(int, posix_siginfo_t *, void *);
};

typedef struct {
	void *ss_sp;
	size_t ss_size;
	int ss_flags;
} posix_stack_t;

typedef struct posix_ucontext {
	struct posix_ucontext *uc_link;
	posix_sigset_t uc_sigmask;
	posix_stack_t uc_stack;
	posix_mcontext_t uc_mcontext;
} posix_ucontext_t;


/* Values of posix_sigevent::sigev_notify */
#undef SIGEV_NONE
#define SIGEV_NONE 0
#undef SIGEV_SIGNAL
#define SIGEV_SIGNAL 0
#undef SIGEV_THREAD
#define SIGEV_THREAD 0

#undef SIGRT_MIN
#define SIGRT_MIN 0
#undef SIGRT_MAX
#define SIGRT_MAX 0

#undef SIG_BLOCK
#define SIG_BLOCK 0
#undef SIG_UNBLOCK
#define SIG_UNBLOCK 0
#undef SIG_SETMASK
#define SIG_SETMASK 0

#undef SA_NOCLDSTOP
#define SA_NOCLDSTOP 0
#undef SA_ONSTACK
#define SA_ONSTACK 0
#undef SA_RESETHAND
#define SA_RESETHAND 0
#undef SA_RESTART
#define SA_RESTART 0
#undef SA_SIGINFO
#define SA_SIGINFO 0
#undef SA_NOCLDWAIT
#define SA_NOCLDWAIT 0
#undef SA_NODEFER
#define SA_NODEFER 0

#undef SS_ONSTACK
#define SS_ONSTACK 0
#undef SS_DISABLE
#define SS_DISABLE 0

#undef MINSIGSTKSZ
#define MINSIGSTKSZ 0
#undef SIGSTKSZ
#define SIGSTKSZ 0

/* full POSIX set */
enum {
	SIGABRT,
	SIGALRM,
	SIGBUS,
	SIGCHLD,
	SIGCONT,
	SIGFPE,
	SIGHUP,
	SIGILL,
	SIGINT,
	SIGKILL,
	SIGPIPE,
	SIGQUIT,
	SIGSEGV,
	SIGSTOP,
	SIGTERM,
	SIGTSTP,
	SIGTTIN,
	SIGTTOU,
	SIGUSR1,
	SIGUSR2,
	SIGPOLL,
	SIGPROF,
	SIGSYS,
	SIGTRAP,
	SIGURG,
	SIGVTALRM,
	SIGXCPU,
	SIGXFSZ
};

#ifndef LIBPOSIX_INTERNAL
	#define sig_atomic_t posix_sig_atomic_t
	#define sigset_t posix_sigset_t
	#define sigval posix_sigval
	#define sigevent posix_sigevent
	#define sigaction posix_sigaction
	#define mcontext_t posix_mcontext_t
	#define ucontext_t posix_ucontext_t
	#define stack_t posix_stack_t
	#define siginfo_t posix_siginfo_t
#endif

#endif /* POSIX_SIGNAL_H_ */

/** @}
 */
