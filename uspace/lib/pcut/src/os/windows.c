/*
 * Copyright (c) 2014 Vojtech Horky
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
 * Windows-specific functions for test execution via the popen() system call.
 */

/*
 * Code inspired by Creating a Child Process with Redirected Input and Output:
 * http://msdn.microsoft.com/en-us/library/ms682499%28VS.85%29.aspx
 */

#include "../internal.h"

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>


/** Maximum size of stdout we are able to capture. */
#define OUTPUT_BUFFER_SIZE 8192

/** Maximum command-line length. */
#define PCUT_COMMAND_LINE_BUFFER_SIZE 256

/** Buffer for assertion and other error messages. */
static char error_message_buffer[OUTPUT_BUFFER_SIZE];

/** Buffer for stdout from the test. */
static char extra_output_buffer[OUTPUT_BUFFER_SIZE];

/** Prepare for a new test.
 *
 * @param test Test that is about to be run.
 */
static void before_test_start(pcut_item_t *test)
{
	pcut_report_test_start(test);

	memset(error_message_buffer, 0, OUTPUT_BUFFER_SIZE);
	memset(extra_output_buffer, 0, OUTPUT_BUFFER_SIZE);
}

/** Report that a certain function failed.
 *
 * @param test Current test.
 * @param failed_function_name Name of the failed function.
 */
static void report_func_fail(pcut_item_t *test, const char *failed_function_name)
{
	/* TODO: get error description. */
	sprintf_s(error_message_buffer, OUTPUT_BUFFER_SIZE - 1,
	    "%s failed: %s.", failed_function_name, "unknown reason");
	pcut_report_test_done(test, TEST_OUTCOME_ERROR, error_message_buffer, NULL, NULL);
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
static size_t read_all(HANDLE fd, char *buffer, size_t buffer_size)
{
	DWORD actually_read;
	char *buffer_start = buffer;
	BOOL okay = FALSE;

	do {
		okay = ReadFile(fd, buffer, buffer_size, &actually_read, NULL);
		if (!okay) {
			break;
		}
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

struct test_output_data {
	HANDLE pipe_stdout;
	HANDLE pipe_stderr;
	char *output_buffer;
	size_t output_buffer_size;
};

static DWORD WINAPI read_test_output_on_background(LPVOID test_output_data_ptr)
{
	size_t stderr_size = 0;
	struct test_output_data *test_output_data = (struct test_output_data *) test_output_data_ptr;

	stderr_size = read_all(test_output_data->pipe_stderr,
	    test_output_data->output_buffer,
	    test_output_data->output_buffer_size - 1);
	read_all(test_output_data->pipe_stdout,
	    test_output_data->output_buffer,
	    test_output_data->output_buffer_size - 1 - stderr_size);

	return 0;
}

/** Run the test as a new process and report the result.
 *
 * @param self_path Path to itself, that is to current binary.
 * @param test Test to be run.
 */
int pcut_run_test_forking(const char *self_path, pcut_item_t *test)
{
	/* TODO: clean-up if something goes wrong "in the middle" */
	BOOL okay = FALSE;
	DWORD rc;
	int outcome;
	int timed_out;
	int time_out_millis;
	SECURITY_ATTRIBUTES security_attributes;
	HANDLE link_stdout[2] = { NULL, NULL };
	HANDLE link_stderr[2] = { NULL, NULL };
	HANDLE link_stdin[2] = { NULL, NULL };
	PROCESS_INFORMATION process_info;
	STARTUPINFO start_info;
	char command[PCUT_COMMAND_LINE_BUFFER_SIZE];
	struct test_output_data test_output_data;
	HANDLE test_output_thread_reader;


	before_test_start(test);

	/* Pipe handles are inherited. */
	security_attributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	security_attributes.bInheritHandle = TRUE;
	security_attributes.lpSecurityDescriptor = NULL;

	/* Create pipe for stdout, make sure it is not inherited. */
	okay = CreatePipe(&link_stdout[0], &link_stdout[1], &security_attributes, 0);
	if (!okay) {
		report_func_fail(test, "CreatePipe(/* stdout */)");
		return PCUT_OUTCOME_INTERNAL_ERROR;
	}
	okay = SetHandleInformation(link_stdout[0], HANDLE_FLAG_INHERIT, 0);
	if (!okay) {
		report_func_fail(test, "SetHandleInformation(/* stdout */)");
		return PCUT_OUTCOME_INTERNAL_ERROR;
	}

	/* Create pipe for stderr, make sure it is not inherited. */
	okay = CreatePipe(&link_stderr[0], &link_stderr[1], &security_attributes, 0);
	if (!okay) {
		report_func_fail(test, "CreatePipe(/* stderr */)");
		return PCUT_OUTCOME_INTERNAL_ERROR;
	}
	okay = SetHandleInformation(link_stderr[0], HANDLE_FLAG_INHERIT, 0);
	if (!okay) {
		report_func_fail(test, "SetHandleInformation(/* stderr */)");
		return PCUT_OUTCOME_INTERNAL_ERROR;
	}

	/* Create pipe for stdin, make sure it is not inherited. */
	okay = CreatePipe(&link_stdin[0], &link_stdin[1], &security_attributes, 0);
	if (!okay) {
		report_func_fail(test, "CreatePipe(/* stdin */)");
		return PCUT_OUTCOME_INTERNAL_ERROR;
	}
	okay = SetHandleInformation(link_stdin[1], HANDLE_FLAG_INHERIT, 0);
	if (!okay) {
		report_func_fail(test, "SetHandleInformation(/* stdin */)");
		return PCUT_OUTCOME_INTERNAL_ERROR;
	}

	/* Prepare information for the child process. */
	ZeroMemory(&process_info, sizeof(PROCESS_INFORMATION));
	ZeroMemory(&start_info, sizeof(STARTUPINFO));
	start_info.cb = sizeof(STARTUPINFO);
	start_info.hStdError = link_stderr[1];
	start_info.hStdOutput = link_stdout[1];
	start_info.hStdInput = link_stdin[0];
	start_info.dwFlags |= STARTF_USESTDHANDLES;

	/* Format the command line. */
	sprintf_s(command, PCUT_COMMAND_LINE_BUFFER_SIZE - 1,
	    "\"%s\" -t%d", self_path, test->id);

	/* Run the process. */
	okay = CreateProcess(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL,
	    &start_info, &process_info);

	if (!okay) {
		report_func_fail(test, "CreateProcess()");
		return PCUT_OUTCOME_INTERNAL_ERROR;
	}

	// FIXME: kill the process on error

	/* Close handles to the first thread. */
	CloseHandle(process_info.hThread);

	/* Close the other ends of the pipes. */
	okay = CloseHandle(link_stdout[1]);
	if (!okay) {
		report_func_fail(test, "CloseHandle(/* stdout */)");
		return PCUT_OUTCOME_INTERNAL_ERROR;
	}
	okay = CloseHandle(link_stderr[1]);
	if (!okay) {
		report_func_fail(test, "CloseHandle(/* stderr */)");
		return PCUT_OUTCOME_INTERNAL_ERROR;
	}
	okay = CloseHandle(link_stdin[0]);
	if (!okay) {
		report_func_fail(test, "CloseHandle(/* stdin */)");
		return PCUT_OUTCOME_INTERNAL_ERROR;
	}

	/*
	 * Read data from stdout and stderr.
	 * We need to do this in a separate thread to allow the
	 * time-out to work correctly.
	 * Probably, this can be done with asynchronous I/O but
	 * this works for now pretty well.
	 */
	test_output_data.pipe_stderr = link_stderr[0];
	test_output_data.pipe_stdout = link_stdout[0];
	test_output_data.output_buffer = extra_output_buffer;
	test_output_data.output_buffer_size = OUTPUT_BUFFER_SIZE;

	test_output_thread_reader = CreateThread(NULL, 0,
	    read_test_output_on_background, &test_output_data,
	    0, NULL);

	if (test_output_thread_reader == NULL) {
		report_func_fail(test, "CreateThread(/* read test stdout */)");
		return PCUT_OUTCOME_INTERNAL_ERROR;
	}

	/* Wait for the process to terminate. */
	timed_out = 0;
	time_out_millis = pcut_get_test_timeout(test) * 1000;
	rc = WaitForSingleObject(process_info.hProcess, time_out_millis);
	PCUT_DEBUG("Waiting for test %s (%dms) returned %d.", test->name, time_out_millis, rc);
	if (rc == WAIT_TIMEOUT) {
		/* We timed-out: kill the process and wait for its termination again. */
		timed_out = 1;
		okay = TerminateProcess(process_info.hProcess, 5);
		if (!okay) {
			report_func_fail(test, "TerminateProcess(/* PROCESS_INFORMATION.hProcess */)");
			return PCUT_ERROR_INTERNAL_FAILURE;
		}
		rc = WaitForSingleObject(process_info.hProcess, INFINITE);
	}
	if (rc != WAIT_OBJECT_0) {
		report_func_fail(test, "WaitForSingleObject(/* PROCESS_INFORMATION.hProcess */)");
		return PCUT_OUTCOME_INTERNAL_ERROR;
	}

	/* Get the return code and convert it to outcome. */
	okay = GetExitCodeProcess(process_info.hProcess, &rc);
	if (!okay) {
		report_func_fail(test, "GetExitCodeProcess()");
		return PCUT_OUTCOME_INTERNAL_ERROR;
	}

	if (rc == 0) {
		outcome = PCUT_OUTCOME_PASS;
	} else if ((rc > 0) && (rc < 10) && !timed_out) {
		outcome = PCUT_OUTCOME_FAIL;
	} else {
		outcome = PCUT_OUTCOME_INTERNAL_ERROR;
	}

	/* Wait for the reader thread (shall be terminated by now). */
	rc = WaitForSingleObject(test_output_thread_reader, INFINITE);
	if (rc != WAIT_OBJECT_0) {
		report_func_fail(test, "WaitForSingleObject(/* stdout reader thread */)");
		return PCUT_ERROR_INTERNAL_FAILURE;
	}

	pcut_report_test_done_unparsed(test, outcome, extra_output_buffer, OUTPUT_BUFFER_SIZE);

	return outcome;
}

void pcut_hook_before_test(pcut_item_t *test)
{
	PCUT_UNUSED(test);

	/*
	 * Prevent displaying the dialog informing the user that the
	 * program unexpectedly failed.
	 */
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
}
