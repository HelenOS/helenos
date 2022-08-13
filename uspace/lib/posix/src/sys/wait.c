/*
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 * SPDX-FileCopyrightText: 2011 Jiri Zarevucky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file Support for waiting.
 */

#include "../internal/common.h"
#include <sys/wait.h>

#include <task.h>
#include <assert.h>

#include <errno.h>

#include <limits.h>
#include <signal.h>

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
	/*
	 * There is no way to distinguish reason
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
