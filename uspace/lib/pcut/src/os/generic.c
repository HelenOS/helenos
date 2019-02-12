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
 * Platform-dependent test execution function when system() is available.
 */
#pragma warning(push, 0)
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#pragma warning(pop)

#include "../internal.h"

/** Maximum command-line length. */
#define PCUT_COMMAND_LINE_BUFFER_SIZE 256

/** Maximum length of a temporary file name. */
#define PCUT_TEMP_FILENAME_BUFFER_SIZE 128

/** Maximum size of stdout we are able to capture. */
#define OUTPUT_BUFFER_SIZE 8192

/* Format the command to launch a test according to OS we are running on. */

#if defined(__WIN64) || defined(__WIN32) || defined(_WIN32)
#include <process.h>

#define FORMAT_COMMAND(buffer, buffer_size, self_path, test_id, temp_file) \
	pcut_snprintf(buffer, buffer_size, "\"\"%s\" -t%d >%s\"", self_path, test_id, temp_file)
#define FORMAT_TEMP_FILENAME(buffer, buffer_size) \
	pcut_snprintf(buffer, buffer_size, "pcut_%d.tmp", _getpid())

#elif defined(__unix)
#include <unistd.h>

#define FORMAT_COMMAND(buffer, buffer_size, self_path, test_id, temp_file) \
	pcut_snprintf(buffer, buffer_size, "%s -t%d &>%s", self_path, test_id, temp_file)
#define FORMAT_TEMP_FILENAME(buffer, buffer_size) \
	pcut_snprintf(buffer, buffer_size, "pcut_%d.tmp", getpid())

#else
#error "Unknown operating system."
#endif

/** Buffer for assertion and other error messages. */
static char error_message_buffer[OUTPUT_BUFFER_SIZE];

/** Buffer for stdout from the test. */
static char extra_output_buffer[OUTPUT_BUFFER_SIZE];

/** Prepare for a new test.
 *
 * @param test Test that is about to start.
 */
static void before_test_start(pcut_item_t *test) {
	pcut_report_test_start(test);

	memset(error_message_buffer, 0, OUTPUT_BUFFER_SIZE);
	memset(extra_output_buffer, 0, OUTPUT_BUFFER_SIZE);
}

/** Convert program exit code to test outcome.
 *
 * @param status Return value from the system() function.
 * @return Test outcome code.
 */
static int convert_wait_status_to_outcome(int status) {
	if (status < 0) {
		return PCUT_OUTCOME_INTERNAL_ERROR;
	} else if (status == 0) {
		return PCUT_OUTCOME_PASS;
	} else {
		return PCUT_OUTCOME_FAIL;
	}
}

/** Run the test as a new process and report the result.
 *
 * @param self_path Path to itself, that is to current binary.
 * @param test Test to be run.
 */
int pcut_run_test_forking(const char *self_path, pcut_item_t *test) {
	int rc, outcome;
	FILE *tempfile;
	char tempfile_name[PCUT_TEMP_FILENAME_BUFFER_SIZE];
	char command[PCUT_COMMAND_LINE_BUFFER_SIZE];

	before_test_start(test);

	FORMAT_TEMP_FILENAME(tempfile_name, PCUT_TEMP_FILENAME_BUFFER_SIZE - 1);
	FORMAT_COMMAND(command, PCUT_COMMAND_LINE_BUFFER_SIZE - 1,
		self_path, (test)->id, tempfile_name);

	PCUT_DEBUG("Will execute <%s> (temp file <%s>) with system().",
		command, tempfile_name);

	rc = system(command);

	PCUT_DEBUG("system() returned 0x%04X", rc);

	outcome = convert_wait_status_to_outcome(rc);

	tempfile = fopen(tempfile_name, "rb");
	if (tempfile == NULL) {
		pcut_report_test_done(test, TEST_OUTCOME_ERROR, "Failed to open temporary file.", NULL, NULL);
		return PCUT_OUTCOME_INTERNAL_ERROR;
	}

	fread(extra_output_buffer, 1, OUTPUT_BUFFER_SIZE, tempfile);
	fclose(tempfile);
	remove(tempfile_name);

	pcut_report_test_done_unparsed(test, outcome, extra_output_buffer, OUTPUT_BUFFER_SIZE);

	return outcome;
}

void pcut_hook_before_test(pcut_item_t *test) {
	PCUT_UNUSED(test);

	/* Do nothing. */
}

