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
 * Implementation of platform-dependent functions for HelenOS.
 */

#include <stdlib.h>
#include <str.h>
#include <str_error.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <task.h>
#include <fibril_synch.h>
#include <vfs/vfs.h>
#include "../internal.h"


/* String functions. */

int pcut_str_equals(const char *a, const char *b) {
	return str_cmp(a, b) == 0;
}


int pcut_str_start_equals(const char *a, const char *b, int len) {
	return str_lcmp(a, b, len) == 0;
}

int pcut_str_size(const char *s) {
	return str_size(s);
}

int pcut_str_to_int(const char *s) {
	int result = strtol(s, NULL, 10);
	return result;
}

char *pcut_str_find_char(const char *haystack, const char needle) {
	return str_chr(haystack, needle);
}

void pcut_str_error(int error, char *buffer, int size) {
	const char *str = str_error(error);
	if (str == NULL) {
		str = "(strerror failure)";
	}
	str_cpy(buffer, size, str);
}


/* Forking-mode related functions. */

/** Maximum width of a test number. */
#define MAX_TEST_NUMBER_WIDTH 24

/** Maximum length of a temporary file name. */
#define PCUT_TEMP_FILENAME_BUFFER_SIZE 128

/** Maximum command-line length. */
#define MAX_COMMAND_LINE_LENGTH 1024

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

/** Mutex guard for forced_termination_cv. */
static FIBRIL_MUTEX_INITIALIZE(forced_termination_mutex);

/** Condition-variable for checking whether test timed-out. */
static FIBRIL_CONDVAR_INITIALIZE(forced_termination_cv);

/** Spawned task id. */
static task_id_t test_task_id;

/** Flag whether test is still running.
 *
 * This flag is used when checking whether test timed-out.
 */
static int test_running;

/** Main fibril for checking whether test timed-out.
 *
 * @param arg Test that is currently running (pcut_item_t *).
 * @return EOK Always.
 */
static int test_timeout_handler_fibril(void *arg) {
	pcut_item_t *test = arg;
	int timeout_sec = pcut_get_test_timeout(test);
	usec_t timeout_us = SEC2USEC(timeout_sec);

	fibril_mutex_lock(&forced_termination_mutex);
	if (!test_running) {
		goto leave_no_kill;
	}
	errno_t rc = fibril_condvar_wait_timeout(&forced_termination_cv,
		&forced_termination_mutex, timeout_us);
	if (rc == ETIMEOUT) {
		task_kill(test_task_id);
	}
leave_no_kill:
	fibril_mutex_unlock(&forced_termination_mutex);
	return EOK;
}

/** Run the test as a new task and report the result.
 *
 * @param self_path Path to itself, that is to current binary.
 * @param test Test to be run.
 */
int pcut_run_test_forking(const char *self_path, pcut_item_t *test) {
	before_test_start(test);

	char tempfile_name[PCUT_TEMP_FILENAME_BUFFER_SIZE];
	snprintf(tempfile_name, PCUT_TEMP_FILENAME_BUFFER_SIZE - 1, "pcut_%lld.tmp", (unsigned long long) task_get_id());
	int tempfile;
	errno_t rc = vfs_lookup_open(tempfile_name, WALK_REGULAR | WALK_MAY_CREATE, MODE_READ | MODE_WRITE, &tempfile);
	if (rc != EOK) {
		pcut_report_test_done(test, PCUT_OUTCOME_INTERNAL_ERROR, "Failed to create temporary file.", NULL, NULL);
		return PCUT_OUTCOME_INTERNAL_ERROR;
	}

	char test_number_argument[MAX_TEST_NUMBER_WIDTH];
	snprintf(test_number_argument, MAX_TEST_NUMBER_WIDTH, "-t%d", test->id);

	const char *const arguments[3] = {
		self_path,
		test_number_argument,
		NULL
	};

	int status = PCUT_OUTCOME_PASS;

	task_wait_t test_task_wait;
	rc = task_spawnvf(&test_task_id, &test_task_wait, self_path, arguments,
	    fileno(stdin), tempfile, tempfile);
	if (rc != EOK) {
		status = PCUT_OUTCOME_INTERNAL_ERROR;
		goto leave_close_tempfile;
	}

	test_running = 1;

	fid_t killer_fibril = fibril_create(test_timeout_handler_fibril, test);
	if (killer_fibril == 0) {
		/* FIXME: somehow announce this problem. */
		task_kill(test_task_id);
	} else {
		fibril_add_ready(killer_fibril);
	}

	task_exit_t task_exit;
	int task_retval;
	rc = task_wait(&test_task_wait, &task_exit, &task_retval);
	if (rc != EOK) {
		status = PCUT_OUTCOME_INTERNAL_ERROR;
		goto leave_close_tempfile;
	}
	if (task_exit == TASK_EXIT_UNEXPECTED) {
		status = PCUT_OUTCOME_INTERNAL_ERROR;
	} else {
		status = task_retval == 0 ? PCUT_OUTCOME_PASS : PCUT_OUTCOME_FAIL;
	}

	fibril_mutex_lock(&forced_termination_mutex);
	test_running = 0;
	fibril_condvar_signal(&forced_termination_cv);
	fibril_mutex_unlock(&forced_termination_mutex);

	aoff64_t pos = 0;
	size_t nread;
	vfs_read(tempfile, &pos, extra_output_buffer, OUTPUT_BUFFER_SIZE, &nread);

leave_close_tempfile:
	vfs_put(tempfile);
	vfs_unlink_path(tempfile_name);

	pcut_report_test_done_unparsed(test, status, extra_output_buffer, OUTPUT_BUFFER_SIZE);

	return status;
}

void pcut_hook_before_test(pcut_item_t *test) {
	PCUT_UNUSED(test);

	/* Do nothing. */
}
