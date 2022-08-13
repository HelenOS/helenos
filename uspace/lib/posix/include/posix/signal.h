/*
 * SPDX-FileCopyrightText: 2011 Jiri Zarevucky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file Signal handling.
 */

#ifndef POSIX_SIGNAL_H_
#define POSIX_SIGNAL_H_

#include <sys/types.h>
#include <ucontext.h>

#define SIG_DFL ((void (*)(int)) __posix_default_signal_handler)
#define SIG_ERR ((void (*)(int)) NULL)
#define SIG_HOLD ((void (*)(int)) __posix_hold_signal_handler)
#define SIG_IGN ((void (*)(int)) __posix_ignore_signal_handler)

/* Values of sigevent::sigev_notify */
#define SIGEV_NONE 0
#define SIGEV_SIGNAL 0
#define SIGEV_THREAD 0

#define SIGRT_MIN 0
#define SIGRT_MAX 0

#define SIG_BLOCK 0
#define SIG_UNBLOCK 1
#define SIG_SETMASK 2

#define SA_NOCLDSTOP (1 << 0)
#define SA_ONSTACK (1 << 1)
#define SA_RESETHAND (1 << 2)
#define SA_RESTART (1 << 3)
#define SA_SIGINFO (1 << 4)
#define SA_NOCLDWAIT (1 << 5)
#define SA_NODEFER (1 << 6)

#define SS_ONSTACK 0
#define SS_DISABLE 0

#define MINSIGSTKSZ 0
#define SIGSTKSZ 0

__C_DECLS_BEGIN;

extern void __posix_default_signal_handler(int signo);
extern void __posix_hold_signal_handler(int signo);
extern void __posix_ignore_signal_handler(int signo);

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

__C_DECLS_END;

#endif /* POSIX_SIGNAL_H_ */

/** @}
 */
