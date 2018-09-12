/*
 * Copyright (c) 2013 Vojtech Horky
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

/** @file
 *
 * Unix-specific functions for test execution via the fork() system call.
 */

/** We need _POSIX_SOURCE because of kill(). */
#define _POSIX_SOURCE
/** We need _BSD_SOURCE because of snprintf() when compiling under C89. */
#define _BSD_SOURCE

/** Newer versions of features.h needs _DEFAULT_SOURCE. */
#define _DEFAULT_SOURCE

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <assert.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include "../internal.h"

/** Maximum size of stdout we are able to capture. */
#define OUTPUT_BUFFER_SIZE 8192

/** Buffer for assertion and other error messages. */
static char error_message_buffer[OUTPUT_BUFFER_SIZE];

/** Buffer for stdout from the test. */
static char extra_output_buffer[OUTPUT_BUFFER_SIZE];

/** Prepare for a new test.
 *
 * @param test Test that is about to be run.
 */
static void before_test_start(pcut_item_t *test) {
	pcut_report_test_start(test);

	memset(error_message_buffer, 0, OUTPUT_BUFFER_SIZE);
	memset(extra_output_buffer, 0, OUTPUT_BUFFER_SIZE);
}

/** PID of the forked process running the actual test. */
static pid_t child_pid;

/** Signal handler that kills the child.
 *
 * @param sig Signal number.
 */
static void kill_child_on_alarm(int sig) {
	PCUT_UNUSED(sig);
	kill(child_pid, SIGKILL);
}

/** Read full buffer from given file descriptor.
 *
 * This function exists to overcome the possibility that read() may
 * not fill the full length of the provided buffer even when EOF is
 * not reached.
 *
 * @param fd Opened file descriptor.
 * @param buffer Buffer to store data into.
 * @param buffer_size Size of the @p buffer in bytes.
 * @return Number of actually read bytes.
 */
static size_t read_all(int fd, char *buffer, size_t buffer_size) {
	ssize_t actually_read;
	char *buffer_start = buffer;
	do {
		actually_read = read(fd, buffer, buffer_size);
		if (actually_read > 0) {
			buffer += actually_read;
			buffer_size -= actually_read;
			if (buffer_size == 0) {
				break;
			}
		}
	} while (actually_read > 0);
	if (buffer_start != buffer) {
		if (*(buffer - 1) == 10) {
			*(buffer - 1) = 0;
			buffer--;
		}
	}
	return buffer - buffer_start;
}

/** Convert program exit code to test outcome.
 *
 * @param status Status value from the wait() function.
 * @return Test outcome code.
 */
static int convert_wait_status_to_outcome(int status) {
	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) != 0) {
			return PCUT_OUTCOME_FAIL;
		} else {
			return PCUT_OUTCOME_PASS;
		}
	}

	if (WIFSIGNALED(status)) {
		return PCUT_OUTCOME_INTERNAL_ERROR;
	}

	return status;
}

/** Run the test in a forked environment and report the result.
 *
 * @param self_path Ignored.
 * @param test Test to be run.
 */
int pcut_run_test_forking(const char *self_path, pcut_item_t *test) {
	int link_stdout[2], link_stderr[2];
	int rc, status, outcome;
	size_t stderr_size;

	PCUT_UNUSED(self_path);

	before_test_start(test);


	rc = pipe(link_stdout);
	if (rc == -1) {
		pcut_snprintf(error_message_buffer, OUTPUT_BUFFER_SIZE - 1,
				"pipe() failed: %s.", strerror(rc));
		pcut_report_test_done(test, PCUT_OUTCOME_INTERNAL_ERROR, error_message_buffer, NULL, NULL);
		return PCUT_OUTCOME_INTERNAL_ERROR;
	}
	rc = pipe(link_stderr);
	if (rc == -1) {
		pcut_snprintf(error_message_buffer, OUTPUT_BUFFER_SIZE - 1,
				"pipe() failed: %s.", strerror(rc));
		pcut_report_test_done(test, PCUT_OUTCOME_INTERNAL_ERROR, error_message_buffer, NULL, NULL);
		return PCUT_OUTCOME_INTERNAL_ERROR;
	}

	child_pid = fork();
	if (child_pid == (pid_t)-1) {
		pcut_snprintf(error_message_buffer, OUTPUT_BUFFER_SIZE - 1,
			"fork() failed: %s.", strerror(rc));
		outcome = PCUT_OUTCOME_INTERNAL_ERROR;
		goto leave_close_pipes;
	}

	if (child_pid == 0) {
		/* We are the child. */
		dup2(link_stdout[1], STDOUT_FILENO);
		close(link_stdout[0]);
		dup2(link_stderr[1], STDERR_FILENO);
		close(link_stderr[0]);

		outcome = pcut_run_test_forked(test);

		exit(outcome);
	}

	close(link_stdout[1]);
	close(link_stderr[1]);

	signal(SIGALRM, kill_child_on_alarm);
	alarm(pcut_get_test_timeout(test));

	stderr_size = read_all(link_stderr[0], extra_output_buffer, OUTPUT_BUFFER_SIZE - 1);
	read_all(link_stdout[0], extra_output_buffer, OUTPUT_BUFFER_SIZE - 1 - stderr_size);

	wait(&status);
	alarm(0);

	outcome = convert_wait_status_to_outcome(status);

	goto leave_close_parent_pipe;

leave_close_pipes:
	close(link_stdout[1]);
	close(link_stderr[1]);
leave_close_parent_pipe:
	close(link_stdout[0]);
	close(link_stderr[0]);

	pcut_report_test_done_unparsed(test, outcome, extra_output_buffer, OUTPUT_BUFFER_SIZE);

	return outcome;
}

void pcut_hook_before_test(pcut_item_t *test) {
	PCUT_UNUSED(test);

	/* Do nothing. */
}

