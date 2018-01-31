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

#include "posix/signal.h"
#include "internal/common.h"
#include "posix/limits.h"
#include "posix/stdlib.h"
#include "posix/string.h"

#include <errno.h>

#include "libc/fibril_synch.h"
#include "libc/task.h"

/* This file implements a fairly dumb and incomplete "simulation" of
 * POSIX signals. Since HelenOS doesn't support signals and mostly doesn't
 * have any equivalent functionality, most of the signals are useless. The
 * main purpose of this implementation is thus to help port applications using
 * signals with minimal modification, but if the application uses signals for
 * anything non-trivial, it's quite probable it won't work properly even if
 * it builds without problems.
 */

/* Used to serialize signal handling. */
static FIBRIL_MUTEX_INITIALIZE(_signal_mutex);

static LIST_INITIALIZE(_signal_queue);

static sigset_t _signal_mask = 0;

#define DEFAULT_HANDLER { .sa_handler = SIG_DFL, \
    .sa_mask = 0, .sa_flags = 0, .sa_sigaction = NULL }

/* Actions associated with each signal number. */
static struct sigaction _signal_actions[_TOP_SIGNAL + 1] = {
	DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, 
	DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, 
	DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, 
	DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, 
	DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, 
	DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, 
	DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER, DEFAULT_HANDLER
};

/**
 * Default signal handler. Executes the default action for each signal,
 * as reasonable within HelenOS.
 *
 * @param signo Signal number.
 */
void __posix_default_signal_handler(int signo)
{
	switch (signo) {
	case SIGABRT:
		abort();
	case SIGQUIT:
		fprintf(stderr, "Quit signal raised. Exiting.\n");
		exit(EXIT_FAILURE);
	case SIGINT:
		fprintf(stderr, "Interrupt signal caught. Exiting.\n");
		exit(EXIT_FAILURE);
	case SIGTERM:
		fprintf(stderr, "Termination signal caught. Exiting.\n");
		exit(EXIT_FAILURE);
	case SIGSTOP:
		fprintf(stderr, "Stop signal caught, but unsupported. Ignoring.\n");
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
		psignal(signo, "Hardware exception raised by user code");
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
		psignal(signo, "Unsupported signal caught");
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

/**
 * Just an empty function to get an unique pointer value for comparison.
 *
 * @param signo Signal number.
 */
void __posix_hold_signal_handler(int signo)
{
	/* Nothing */
}

/**
 * Empty function to be used as ignoring handler.
 * 
 * @param signo Signal number.
 */
void __posix_ignore_signal_handler(int signo)
{
	/* Nothing */
}

/**
 * Clear the signal set.
 * 
 * @param set Pointer to the signal set.
 * @return Always returns zero.
 */
int sigemptyset(sigset_t *set)
{
	assert(set != NULL);

	*set = 0;
	return 0;
}

/**
 * Fill the signal set (i.e. add all signals).
 * 
 * @param set Pointer to the signal set.
 * @return Always returns zero.
 */
int sigfillset(sigset_t *set)
{
	assert(set != NULL);

	*set = UINT32_MAX;
	return 0;
}

/**
 * Add a signal to the set.
 * 
 * @param set Pointer to the signal set.
 * @param signo Signal number to add.
 * @return Always returns zero.
 */
int sigaddset(sigset_t *set, int signo)
{
	assert(set != NULL);

	*set |= (1 << signo);
	return 0;
}

/**
 * Delete a signal from the set.
 * 
 * @param set Pointer to the signal set.
 * @param signo Signal number to remove.
 * @return Always returns zero.
 */
int sigdelset(sigset_t *set, int signo)
{
	assert(set != NULL);

	*set &= ~(1 << signo);
	return 0;
}

/**
 * Inclusion test for a signal set.
 * 
 * @param set Pointer to the signal set.
 * @param signo Signal number to query.
 * @return 1 if the signal is in the set, 0 otherwise.
 */
int sigismember(const sigset_t *set, int signo)
{
	assert(set != NULL);
	
	return (*set & (1 << signo)) != 0;
}

/**
 * Unsafe variant of the sigaction() function.
 * Doesn't do any checking of its arguments and
 * does not deal with thread-safety.
 * 
 * @param sig
 * @param act
 * @param oact
 */
static void _sigaction_unsafe(int sig, const struct sigaction *restrict act,
    struct sigaction *restrict oact)
{
	if (oact != NULL) {
		memcpy(oact, &_signal_actions[sig],
		    sizeof(struct sigaction));
	}

	if (act != NULL) {
		memcpy(&_signal_actions[sig], act,
		    sizeof(struct sigaction));
	}
}

/**
 * Sets a new action for the given signal number.
 * 
 * @param sig Signal number to set action for.
 * @param act If not NULL, contents of this structure are
 *     used as the new action for the signal.
 * @param oact If not NULL, the original action associated with the signal
 *     is stored in the structure pointer to. 
 * @return -1 with errno set on failure, 0 on success.
 */
int sigaction(int sig, const struct sigaction *restrict act,
    struct sigaction *restrict oact)
{
	if (sig > _TOP_SIGNAL || (act != NULL &&
	    (sig == SIGKILL || sig == SIGSTOP))) {
		errno = EINVAL;
		return -1;
	}

	if (sig > _TOP_CATCHABLE_SIGNAL) {
		psignal(sig,
		    "WARNING: registering handler for a partially"
		    " or fully unsupported signal. This handler may only be"
		    " invoked by the raise() function, which may not be what"
		    " the application developer intended");
	}

	fibril_mutex_lock(&_signal_mutex);
	_sigaction_unsafe(sig, act, oact);
	fibril_mutex_unlock(&_signal_mutex);

	return 0;
}

/**
 * Sets a new handler for the given signal number.
 * 
 * @param sig Signal number to set handler for.
 * @param func Handler function.
 * @return SIG_ERR on failure, original handler on success.
 */
void (*signal(int sig, void (*func)(int)))(int)
{
	struct sigaction new = {
		.sa_handler = func,
		.sa_mask = 0,
		.sa_flags = 0,
		.sa_sigaction = NULL
	};
	struct sigaction old;
	if (sigaction(sig, func == NULL ? NULL : &new, &old) == 0) {
		return old.sa_handler;
	} else {
		return SIG_ERR;
	}
}

typedef struct {
	link_t link;
	int signo;
	siginfo_t siginfo;
} signal_queue_item;

/**
 * Queue blocked signal.
 *
 * @param signo Signal number.
 * @param siginfo Additional information about the signal.
 */
static void _queue_signal(int signo, siginfo_t *siginfo)
{
	assert(signo >= 0 && signo <= _TOP_SIGNAL);
	assert(siginfo != NULL);
	
	signal_queue_item *item = malloc(sizeof(signal_queue_item));
	link_initialize(&(item->link));
	item->signo = signo;
	memcpy(&item->siginfo, siginfo, sizeof(siginfo_t));
	list_append(&(item->link), &_signal_queue);
}


/**
 * Executes an action associated with the given signal.
 *
 * @param signo Signal number.
 * @param siginfo Additional information about the circumstances of this raise.
 * @return 0 if the action has been successfully executed. -1 if the signal is
 *     blocked.
 */
static int _raise_sigaction(int signo, siginfo_t *siginfo)
{
	assert(signo >= 0 && signo <= _TOP_SIGNAL);
	assert(siginfo != NULL);

	fibril_mutex_lock(&_signal_mutex);

	struct sigaction action = _signal_actions[signo];

	if (sigismember(&_signal_mask, signo) ||
	    action.sa_handler == SIG_HOLD) {
		_queue_signal(signo, siginfo);
		fibril_mutex_unlock(&_signal_mutex);
		return -1;
	}

	/* Modifying signal mask is unnecessary,
	 * signal handling is serialized.
	 */

	if ((action.sa_flags & SA_RESETHAND) && signo != SIGILL && signo != SIGTRAP) {
		_signal_actions[signo] = (struct sigaction) DEFAULT_HANDLER;
	}

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

/**
 * Raise all unblocked previously queued signals.
 */
static void _dequeue_unblocked_signals(void)
{
	link_t *iterator = _signal_queue.head.next;
	link_t *next;
	
	while (iterator != &(_signal_queue).head) {
		next = iterator->next;
		
		signal_queue_item *item =
		    list_get_instance(iterator, signal_queue_item, link);
		
		if (!sigismember(&_signal_mask, item->signo) &&
		    _signal_actions[item->signo].sa_handler != SIG_HOLD) {
			list_remove(&(item->link));
			_raise_sigaction(item->signo, &(item->siginfo));
			free(item);
		}
		
		iterator = next;
	}
}

/**
 * Raise a signal for the calling process.
 * 
 * @param sig Signal number.
 * @return -1 with errno set on failure, 0 on success.
 */
int raise(int sig)
{
	if (sig >= 0 && sig <= _TOP_SIGNAL) {
		siginfo_t siginfo = {
			.si_signo = sig,
			.si_code = SI_USER
		};
		return _raise_sigaction(sig, &siginfo);
	} else {
		errno = EINVAL;
		return -1;
	}
}

/**
 * Raises a signal for a selected process.
 * 
 * @param pid PID of the process for which the signal shall be raised.
 * @param signo Signal to raise.
 * @return -1 with errno set on failure (possible errors include unsupported
 *     action, invalid signal number, lack of permissions, etc.), 0 on success.
 */
int kill(pid_t pid, int signo)
{
	if (pid < 1) {
		// TODO
		errno = ENOTSUP;
		return -1;
	}

	if (signo > _TOP_SIGNAL) {
		errno = EINVAL;
		return -1;
	}

	if (pid == (pid_t) task_get_id()) {
		return raise(signo);
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

/**
 * Send a signal to a process group. Always fails at the moment because of
 * lack of this functionality in HelenOS.
 * 
 * @param pid PID of the process group.
 * @param sig Signal number.
 * @return -1 on failure, 0 on success (see kill()).
 */
int killpg(pid_t pid, int sig)
{
	assert(pid > 1);
	return kill(-pid, sig);
}

/**
 * Outputs information about the signal to the standard error stream.
 * 
 * @param pinfo SigInfo struct to write.
 * @param message String to output alongside human-readable signal description.
 */
void psiginfo(const siginfo_t *pinfo, const char *message)
{
	assert(pinfo != NULL);
	psignal(pinfo->si_signo, message);
	// TODO: print si_code
}

/**
 * Outputs information about the signal to the standard error stream.
 * 
 * @param signum Signal number.
 * @param message String to output alongside human-readable signal description.
 */
void psignal(int signum, const char *message)
{
	char *sigmsg = strsignal(signum);
	if (message == NULL || *message == '\0') {
		fprintf(stderr, "%s\n", sigmsg);
	} else {
		fprintf(stderr, "%s: %s\n", message, sigmsg);
	}
}

/**
 * Manipulate the signal mask of the calling thread.
 * 
 * @param how What to do with the mask.
 * @param set Signal set to work with.
 * @param oset If not NULL, the original signal mask is coppied here.
 * @return 0 success, errorcode on failure.
 */
int thread_sigmask(int how, const sigset_t *restrict set,
    sigset_t *restrict oset)
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
	
	_dequeue_unblocked_signals();

	fibril_mutex_unlock(&_signal_mutex);

	return 0;
}

/**
 * Manipulate the signal mask of the process.
 * 
 * @param how What to do with the mask.
 * @param set Signal set to work with.
 * @param oset If not NULL, the original signal mask is coppied here.
 * @return 0 on success, -1 with errno set on failure.
 */
int sigprocmask(int how, const sigset_t *restrict set,
    sigset_t *restrict oset)
{
	int result = thread_sigmask(how, set, oset);
	if (result != 0) {
		errno = result;
		return -1;
	}
	return 0;
}

/** @}
 */
