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
/** @file Signal handling.
 */

#ifndef POSIX_SIGNAL_H_
#define POSIX_SIGNAL_H_

#include "libc/errno.h"
#include "sys/types.h"

extern void __posix_default_signal_handler(int signo);
extern void __posix_hold_signal_handler(int signo);
extern void __posix_ignore_signal_handler(int signo);

#undef SIG_DFL
#define SIG_DFL ((void (*)(int)) __posix_default_signal_handler)
#undef SIG_ERR
#define SIG_ERR ((void (*)(int)) NULL)
#undef SIG_HOLD
#define SIG_HOLD ((void (*)(int)) __posix_hold_signal_handler)
#undef SIG_IGN
#define SIG_IGN ((void (*)(int)) __posix_ignore_signal_handler)

typedef int posix_sig_atomic_t;
typedef uint32_t posix_sigset_t;
typedef struct posix_mcontext {
	/* must not be empty to avoid compiler warnings (-pedantic) */
	int dummy;
} posix_mcontext_t;

union posix_sigval {
	int sival_int;
	void *sival_ptr;
};

struct posix_sigevent {
	int sigev_notify; /* Notification type. */
	int sigev_signo; /* Signal number. */
	union posix_sigval sigev_value; /* Signal value. */
	void (*sigev_notify_function)(union posix_sigval); /* Notification function. */
	posix_thread_attr_t *sigev_notify_attributes; /* Notification attributes. */
};

typedef struct {
	int si_signo;
	int si_code;

	int si_errno;

	posix_pid_t si_pid;
	posix_uid_t si_uid;
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
#undef SIGEV_SIGNAL
#undef SIGEV_THREAD
#define SIGEV_NONE 0
#define SIGEV_SIGNAL 0
#define SIGEV_THREAD 0

#undef SIGRT_MIN
#undef SIGRT_MAX
#define SIGRT_MIN 0
#define SIGRT_MAX 0

#undef SIG_BLOCK
#undef SIG_UNBLOCK
#undef SIG_SETMASK
#define SIG_BLOCK 0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

#undef SA_NOCLDSTOP
#undef SA_ONSTACK
#undef SA_RESETHAND
#undef SA_RESTART
#undef SA_SIGINFO
#undef SA_NOCLDWAIT
#undef SA_NODEFER
#define SA_NOCLDSTOP (1 << 0)
#define SA_ONSTACK (1 << 1)
#define SA_RESETHAND (1 << 2)
#define SA_RESTART (1 << 3)
#define SA_SIGINFO (1 << 4)
#define SA_NOCLDWAIT (1 << 5)
#define SA_NODEFER (1 << 6)

#undef SS_ONSTACK
#undef SS_DISABLE
#define SS_ONSTACK 0
#define SS_DISABLE 0

#undef MINSIGSTKSZ
#undef SIGSTKSZ
#define MINSIGSTKSZ 0
#define SIGSTKSZ 0

/* Full POSIX set */
enum {
	/* Termination Signals */
	SIGABRT,
	SIGQUIT,
	SIGINT,
	SIGTERM,
	
	/* Child Signal */
	SIGCHLD,
	
	/* User signals */
	SIGUSR1,
	SIGUSR2,

	/* Timer */
	SIGALRM,
	SIGVTALRM,
	SIGPROF, /* obsolete */

	_TOP_CATCHABLE_SIGNAL = SIGPROF,

	/* Process Scheduler Interaction - not supported */
	SIGSTOP,
	SIGCONT,

	/* Process Termination - can't be caught */
	SIGKILL,

	_TOP_SENDABLE_SIGNAL = SIGKILL,

	/* Hardware Exceptions - can't be caught or sent */
	SIGFPE,
	SIGBUS,
	SIGILL,
	SIGSEGV,

	/* Other Exceptions - not supported */
	SIGSYS,
	SIGXCPU,
	SIGXFSZ,

	/* Debugging - not supported */
	SIGTRAP,

	/* Communication Signals - not supported */
	SIGHUP,
	SIGPIPE,
	SIGPOLL, /* obsolete */
	SIGURG,

	/* Terminal Signals - not supported */
	SIGTSTP,
	SIGTTIN,
	SIGTTOU,
	
	_TOP_SIGNAL = SIGTTOU
};

/* Values for sigaction field si_code. */
enum {
	SI_USER,
	SI_QUEUE,
	SI_TIMER,
	SI_ASYNCIO,
	SI_MESGQ,
	ILL_ILLOPC,
	ILL_ILLOPN,
	ILL_ILLADR,
	ILL_ILLTRP,
	ILL_PRVOPC,
	ILL_PRVREG,
	ILL_COPROC,
	ILL_BADSTK,
	FPE_INTDIV,
	FPE_INTOVF,
	FPE_FLTDIV,
	FPE_FLTOVF,
	FPE_FLTUND,
	FPE_FLTRES,
	FPE_FLTINV,
	FPE_FLTSUB,
	SEGV_MAPERR,
	SEGV_ACCERR,
	BUS_ADRALN,
	BUS_ADRERR,
	BUS_OBJERR,
	TRAP_BRKPT,
	TRAP_TRACE,
	CLD_EXITED,
	CLD_KILLED,
	CLD_DUMPED,
	CLD_TRAPPED,
	CLD_STOPPED,
	CLD_CONTINUED,
	POLL_IN,
	POLL_OUT,
	POLL_MSG,
	POLL_ERR,
	POLL_PRI,
	POLL_HUP
};

extern int posix_sigaction(int sig, const struct posix_sigaction *restrict act,
    struct posix_sigaction *restrict oact);

extern void (*posix_signal(int sig, void (*func)(int)))(int);
extern int posix_raise(int sig);
extern int posix_kill(posix_pid_t pid, int sig);
extern int posix_killpg(posix_pid_t pid, int sig);

extern void posix_psiginfo(const posix_siginfo_t *pinfo, const char *message);
extern void posix_psignal(int signum, const char *message);

extern int posix_sigemptyset(posix_sigset_t *set);
extern int posix_sigfillset(posix_sigset_t *set);
extern int posix_sigaddset(posix_sigset_t *set, int signo);
extern int posix_sigdelset(posix_sigset_t *set, int signo);
extern int posix_sigismember(const posix_sigset_t *set, int signo);

extern int posix_thread_sigmask(int how, const posix_sigset_t *restrict set,
    posix_sigset_t *restrict oset);
extern int posix_sigprocmask(int how, const posix_sigset_t *restrict set,
    posix_sigset_t *restrict oset);

#ifndef LIBPOSIX_INTERNAL
	#define sig_atomic_t posix_sig_atomic_t
	#define sigset_t posix_sigset_t
	#define sigval posix_sigval
	#ifndef sigevent
		#define sigevent posix_sigevent
	#endif
	#define mcontext_t posix_mcontext_t
	#define ucontext_t posix_ucontext_t
	#define stack_t posix_stack_t
	#define siginfo_t posix_siginfo_t

	#define sigaction posix_sigaction

	#define signal posix_signal
	#define raise posix_raise
	#define kill posix_kill
	#define killpg posix_killpg

	#define psiginfo posix_psiginfo
	#define psignal posix_psignal

	#define sigemptyset posix_sigemptyset
	#define sigfillset posix_sigfillset
	#define sigaddset posix_sigaddset
	#define sigdelset posix_sigdelset
	#define sigismember posix_sigismember

	#define pthread_sigmask posix_thread_sigmask
	#define sigprocmask posix_sigprocmask
#endif

#endif /* POSIX_SIGNAL_H_ */

/** @}
 */
