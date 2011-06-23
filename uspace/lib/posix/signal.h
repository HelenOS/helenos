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
#undef SIG_IGN
#define SIG_IGN ((void (*)(int)) 0)

#define signal(sig,func) (errno = ENOTSUP, SIG_ERR)
#define raise(sig) ((int) -1)
#define kill(pid,sig) (errno = ENOTSUP, (int) -1)

typedef int posix_sig_atomic_t;

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
#endif

#endif /* POSIX_SIGNAL_H_ */

/** @}
 */
