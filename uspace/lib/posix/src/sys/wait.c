/*
 * Copyright (c) 2011 Petr Koupy
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
/** @file Support for waiting.
 */

#include "../internal/common.h"
#include "posix/sys/wait.h"

#include "libc/task.h"
#include <assert.h>

#include <errno.h>

#include "posix/limits.h"
#include "posix/signal.h"

int __posix_wifexited(int status)
{
	return status != INT_MIN;
}

int __posix_wexitstatus(int status)
{
	assert(__posix_wifexited(status));
	return status;
}

int __posix_wifsignaled(int status)
{
	return status == INT_MIN;
}

int __posix_wtermsig(int status)
{
	assert(__posix_wifsignaled(status));
	/* There is no way to distinguish reason
	 * for unexpected termination at the moment.
	 */
	return SIGABRT;
}

/**
 * Wait for any child process to stop or terminate.
 *
 * @param stat_ptr Location of the final status code of the child process.
 * @return ID of the child process for which status is reported,
 *     -1 on signal interrupt, (pid_t)-1 otherwise.
 */
pid_t wait(int *stat_ptr)
{
	/* HelenOS does not support this. */
	errno = ENOSYS;
	return (pid_t) -1;
}

/**
 * Wait for a child process to stop or terminate.
 *
 * @param pid What child process shall the caller wait for. See POSIX manual
 *     for details.
 * @param stat_ptr Location of the final status code of the child process.
 * @param options Constraints of the waiting. See POSIX manual for details.
 * @return ID of the child process for which status is reported,
 *     -1 on signal interrupt, 0 if non-blocking wait is requested but there is
 *     no child process whose status can be reported, (pid_t)-1 otherwise.
 */
pid_t waitpid(pid_t pid, int *stat_ptr, int options)
{
	assert(stat_ptr != NULL);
	assert(options == 0 /* None of the options are supported. */);

	task_exit_t texit;
	int retval;

	if (failed(task_wait_task_id((task_id_t) pid, &texit, &retval))) {
		/* Unable to retrieve status. */
		return (pid_t) -1;
	}

	if (texit == TASK_EXIT_NORMAL) {
		// FIXME: relies on application not returning this value
		assert(retval != INT_MIN);
		*stat_ptr = retval;
	} else {
		/* Reserve the lowest value for unexpected termination. */
		*stat_ptr = INT_MIN;
	}

	return pid;
}

/** @}
 */
