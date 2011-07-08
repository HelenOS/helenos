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

#define LIBPOSIX_INTERNAL

#include "signal.h"
#include "internal/common.h"
#include "limits.h"
#include "stdlib.h"
#include "string.h"
#include "errno.h"

#include "libc/fibril_synch.h"
#include "libc/task.h"

/* Used to serialize signal handling. */
static FIBRIL_MUTEX_INITIALIZE(_signal_mutex);

static posix_sigset_t _signal_mask = 0;

#define DEFAULT_HANDLER { .sa_handler = SIG_DFL, \
    .sa_mask = 0, .sa_flags = 0, .sa_sigaction = NULL }

static struct posix_sigaction _signal_actions[_TOP_SIGNAL + 1] = {
	DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, 
	DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, 
	DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, 
	DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, 
	DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, 
	DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, 
	DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER
};

void __posix_default_signal_handler(int signo)
{
	switch (signo) {
	case SIGABRT:
		abort();
	case SIGQUIT:
		fprintf(stderr, "Quit signal raised. Exiting.");
		exit(EXIT_FAILURE);
	case SIGINT:
		fprintf(stderr, "Interrupt signal caught. Exiting.");
		exit(EXIT_FAILURE);
	case SIGTERM:
		fprintf(stderr, "Termination signal caught. Exiting.");
		exit(EXIT_FAILURE);
	case SIGSTOP:
		fprintf(stderr, "Stop signal caught, but unsupported. Ignoring.");
		break;
	case SIGKILL:
		/* This will only occur when raise or similar is called. */
		/* Commit suicide. */
		task_kill(task_get_id());
		
		/* Should not be reached. */
		abort();
	case SIGFPE:
	case SIGBUS:
	case SIGILL:
	case SIGSEGV:
		posix_psignal(signo, "Hardware exception raised by user code");
		abort();
	case SIGSYS:
	case SIGXCPU:
	case SIGXFSZ:
	case SIGTRAP:
	case SIGHUP:
	case SIGPIPE:
	case SIGPOLL:
	case SIGURG:
	case SIGTSTP:
	case SIGTTIN:
	case SIGTTOU:
		posix_psignal(signo, "Unsupported signal caught");
		abort();
	case SIGCHLD:
	case SIGUSR1:
	case SIGUSR2:
	case SIGALRM:
	case SIGVTALRM:
	case SIGPROF:
	case SIGCONT:
		/* ignored */
		break;
	}
}

void __posix_hold_signal_handler(int signo)
{
	/* Nothing */
}

void __posix_ignore_signal_handler(int signo)
{
	/* Nothing */
}


int posix_sigemptyset(posix_sigset_t *set)
{
	assert(set != NULL);

	*set = 0;
	return 0;
}

int posix_sigfillset(posix_sigset_t *set)
{
	assert(set != NULL);

	*set = UINT32_MAX;
	return 0;
}

int posix_sigaddset(posix_sigset_t *set, int signo)
{
	assert(set != NULL);

	*set |= (1 << signo);
	return 0;
}

int posix_sigdelset(posix_sigset_t *set, int signo)
{
	assert(set != NULL);

	*set &= ~(1 << signo);
	return 0;
}

int posix_sigismember(const posix_sigset_t *set, int signo)
{
	assert(set != NULL);
	
	return (*set & (1 << signo)) != 0;
}

static void _sigaction_unsafe(int sig, const struct posix_sigaction *restrict act,
    struct posix_sigaction *restrict oact)
{
	if (oact != NULL) {
		memcpy(oact, &_signal_actions[sig],
		    sizeof(struct posix_sigaction));
	}

	if (act != NULL) {
		memcpy(&_signal_actions[sig], act,
		    sizeof(struct posix_sigaction));
	}
}

int posix_sigaction(int sig, const struct posix_sigaction *restrict act,
    struct posix_sigaction *restrict oact)
{
	if (sig > _TOP_SIGNAL || (act != NULL &&
	    (sig == SIGKILL || sig == SIGSTOP))) {
		errno = EINVAL;
		return -1;
	}

	if (sig > _TOP_CATCHABLE_SIGNAL) {
		posix_psignal(sig,
		    "WARNING: registering handler for a partially"
		    " or fully unsupported signal. This handler may only be"
		    " invoked by the raise() function, which may not be what"
		    " the application developer intended.\nSignal name");
	}

	fibril_mutex_lock(&_signal_mutex);
	_sigaction_unsafe(sig, act, oact);
	fibril_mutex_unlock(&_signal_mutex);

	return 0;
}

void (*posix_signal(int sig, void (*func)(int)))(int)
{
	struct posix_sigaction new = {
		.sa_handler = func,
		.sa_mask = 0,
		.sa_flags = 0,
		.sa_sigaction = NULL
	};
	struct posix_sigaction old;
	if (posix_sigaction(sig, func == NULL ? NULL : &new, &old) == 0) {
		return old.sa_handler;
	} else {
		return SIG_ERR;
	}
}

static int _raise_sigaction(int signo, posix_siginfo_t *siginfo)
{
	assert(signo >= 0 && signo <= _TOP_SIGNAL);
	assert(siginfo != NULL);

	fibril_mutex_lock(&_signal_mutex);

	struct posix_sigaction action = _signal_actions[signo];

	if (posix_sigismember(&_signal_mask, signo) ||
	    action.sa_handler == SIG_HOLD) {
		// TODO: queue signal
		fibril_mutex_unlock(&_signal_mutex);
		return -1;
	}

	/* Modifying signal mask is unnecessary,
	 * signal handling is serialized.
	 */

	if ((action.sa_flags & SA_RESETHAND) && signo != SIGILL && signo != SIGTRAP) {
		_signal_actions[signo] = (struct posix_sigaction) DEFAULT_HANDLER;
	};

	if (action.sa_flags & SA_SIGINFO) {
		assert(action.sa_sigaction != NULL);
		action.sa_sigaction(signo, siginfo, NULL);
	} else {
		assert(action.sa_handler != NULL);
		action.sa_handler(signo);
	}

	fibril_mutex_unlock(&_signal_mutex);

	return 0;
}

int posix_raise(int sig)
{
	if (sig >= 0 && sig <= _TOP_SIGNAL) {
		posix_siginfo_t siginfo = {
			.si_signo = sig,
			.si_code = SI_USER
		};
		return _raise_sigaction(sig, &siginfo);
	} else {
		errno = EINVAL;
		return -1;
	}
}

int posix_kill(posix_pid_t pid, int signo)
{
	if (pid < 1) {
		// TODO
		errno = ENOTSUP;
		return -1;
	}

	if (pid == task_get_id()) {
		return posix_raise(signo);
	}

	if (pid > _TOP_SIGNAL) {
		errno = EINVAL;
		return -1;
	}

	switch (signo) {
	case SIGKILL:
		task_kill(pid);
		break;
	default:
		/* Nothing else supported yet. */
		errno = ENOTSUP;
		return -1;
	}

	return 0;
}

int posix_killpg(posix_pid_t pid, int sig)
{
	assert(pid > 1);
	return posix_kill(-pid, sig);
}

void posix_psiginfo(const posix_siginfo_t *pinfo, const char *message)
{
	assert(pinfo != NULL);
	posix_psignal(pinfo->si_signo, message);
	// TODO: print si_code
}

void posix_psignal(int signum, const char *message)
{
	char *sigmsg = posix_strsignal(signum);
	if (message == NULL || *message == '\0') {
		fprintf(stderr, "%s\n", sigmsg);
	} else {
		fprintf(stderr, "%s: %s\n", message, sigmsg);
	}
}

int posix_thread_sigmask(int how, const posix_sigset_t *restrict set,
    posix_sigset_t *restrict oset)
{
	fibril_mutex_lock(&_signal_mutex);

	if (oset != NULL) {
		*oset = _signal_mask;
	}
	if (set != NULL) {
		switch (how) {
		case SIG_BLOCK:
			_signal_mask |= *set;
			break;
		case SIG_UNBLOCK:
			_signal_mask &= ~*set;
			break;
		case SIG_SETMASK:
			_signal_mask = *set;
			break;
		default:
			fibril_mutex_unlock(&_signal_mutex);
			return EINVAL;
		}
	}

	// TODO: queued signal handling

	fibril_mutex_unlock(&_signal_mutex);

	return 0;
}

int posix_sigprocmask(int how, const posix_sigset_t *restrict set,
    posix_sigset_t *restrict oset)
{
	int result = posix_thread_sigmask(how, set, oset);
	if (result != 0) {
		errno = result;
		return -1;
	}
	return 0;
}

/** @}
 */

