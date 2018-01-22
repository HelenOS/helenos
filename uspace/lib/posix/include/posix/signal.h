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

#include "sys/types.h"
#include <posix/ucontext.h>

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


typedef struct {
	int si_signo;
	int si_code;

	int si_errno;

	pid_t si_pid;
	uid_t si_uid;
	void *si_addr;
	int si_status;

	long si_band;

	union sigval si_value;
} siginfo_t;

struct sigaction {
	void (*sa_handler)(int);
	sigset_t sa_mask;
	int sa_flags;
	void (*sa_sigaction)(int, siginfo_t *, void *);
};


/* Values of sigevent::sigev_notify */
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

extern int sigaction(int sig, const struct sigaction *__restrict__ act,
    struct sigaction *__restrict__ oact);

extern void (*signal(int sig, void (*func)(int)))(int);
extern int raise(int sig);
extern int kill(pid_t pid, int sig);
extern int killpg(pid_t pid, int sig);

extern void psiginfo(const siginfo_t *pinfo, const char *message);
extern void psignal(int signum, const char *message);

extern int sigemptyset(sigset_t *set);
extern int sigfillset(sigset_t *set);
extern int sigaddset(sigset_t *set, int signo);
extern int sigdelset(sigset_t *set, int signo);
extern int sigismember(const sigset_t *set, int signo);

extern int thread_sigmask(int how, const sigset_t *__restrict__ set,
    sigset_t *__restrict__ oset);
extern int sigprocmask(int how, const sigset_t *__restrict__ set,
    sigset_t *__restrict__ oset);


#endif /* POSIX_SIGNAL_H_ */

/** @}
 */
