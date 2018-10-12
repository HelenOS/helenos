/*
 * Copyright (c) 2018 Jiří Zárevúcky
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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>
#include <task.h>
#include <vfs/vfs.h>
#include <dirent.h>
#include <str.h>

static errno_t run_test(const char *logfile, const char *logmode,
    const char *path, const char *const args[], task_exit_t *ex, int *retval)
{
	FILE *f = fopen(logfile, logmode);
	if (!f) {
		fprintf(stderr, "Can't open file %s: %s\n",
		    logfile, str_error(errno));
		return errno;
	}

	int h;
	errno_t rc = vfs_fhandle(f, &h);
	if (rc != EOK) {
		fprintf(stderr, "Error getting file handle: %s\n",
		    str_error_name(rc));
		fclose(f);
		return rc;
	}

	task_id_t id;
	task_wait_t wait;

	rc = task_spawnvf(&id, &wait, path, args, -1, h, h);
	if (rc != EOK) {
		fprintf(stderr, "Task spawning failed: %s\n",
		    str_error_name(rc));
		fclose(f);
		return rc;
	}

	rc = task_wait(&wait, ex, retval);
	if (rc != EOK) {
		fprintf(stderr, "Task wait failed: %s\n",
		    str_error_name(rc));
		fclose(f);
		return rc;
	}

	// TODO: check that we are managing resources correctly
	fclose(f);
	return EOK;
}

static void run_tester(const char *logfile)
{
	const char *const tests[] = {
		"thread1",
		"setjmp1",
		"print1",
		"print2",
		"print3",
		"print4",
		"print5",
		"print6",
		"stdio1",
		"stdio2",
		"logger1",
		"fault1",
		"fault2",
		"fault3",
		"float1",
		"float2",
		"vfs1",
		"ping_pong",
		"malloc1",
		// FIXME: malloc2 doesn't work as expected
		// "malloc2",
		"malloc3",
		"mapping1",
		"pager1",
		// "serial1",
		// "chardev1",
	};

	int tests_count = sizeof(tests) / sizeof(const char *);

	task_exit_t ex;
	int retval;
	int failed = 0;

	const char *app = "/app/tester";

	for (int i = 0; i < tests_count; i++) {
		const char *const args[] = { app, tests[i], NULL };
		errno_t rc = run_test(logfile, "a", app, args, &ex, &retval);
		if (rc != EOK) {
			/* Reason already printed in run_test(). */
			continue;
		}

		if (ex != TASK_EXIT_NORMAL) {
			fprintf(stderr, "tester %s CRASHED\n",
			    tests[i]);
			failed++;
			continue;
		}

		if (retval == 0) {
			printf("tester %s ok\n", tests[i]);
		} else {
			printf("tester %s FAILED\n", tests[i]);
			failed++;
		}
	}

	printf("tester: %d failed tests\n", failed);
}

static void run_tester_fault(const char *logfile)
{
	task_exit_t ex;
	int retval;

	const char *app = "/app/tester";
	const char *const args[] = { app, "fault1", NULL };
	errno_t rc = run_test(logfile, "w", app, args, &ex, &retval);
	if (rc != EOK) {
		/* Reason already printed in run_test(). */
		return;
	}

	if (ex != TASK_EXIT_UNEXPECTED) {
		fprintf(stderr, "`tester fault1` unexpectedly"
		    " didn't terminate unexpectedly\n");
		return;
	}

	printf("`tester fault1`: terminated as expected\n");
}

static void run_pcut_tests(void)
{
	printf("Running all pcut tests...\n");

	DIR *d = opendir("/test");
	if (!d)
		return;

	struct dirent *e;

	while ((e = readdir(d))) {
		if (str_lcmp(e->d_name, "test-", 5) != 0)
			continue;

		task_exit_t ex;
		int retval;

		char *bin;
		if (asprintf(&bin, "/test/%s", e->d_name) < 0) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}

		char *logfile;
		if (asprintf(&logfile, "/data/web/result-%s.txt", e->d_name) < 0) {
			fprintf(stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}

		const char *const args[] = { bin, NULL };
		errno_t rc = run_test(logfile, "w", bin, args, &ex, &retval);
		free(bin);
		free(logfile);

		if (rc != EOK) {
			/* Reason already printed in run_test(). */
			return;
		}

		if (ex != TASK_EXIT_NORMAL) {
			fprintf(stderr, "%s CRASHED\n", e->d_name);
			return;
		}

		if (retval == 0) {
			printf("%s ok\n", e->d_name);
		} else {
			printf("%s FAILED\n", e->d_name);
		}
	}

	closedir(d);
}

static void gen_index(const char *fname)
{
	FILE *f = fopen(fname, "w");
	if (!f) {
		fprintf(stderr, "Can't open %s for writing: %s\n", fname,
		    str_error_name(errno));
		return;
	}

	fprintf(f, "<html><head><title>HelenOS test results"
	    "</title></head><body>\n");
	fprintf(f, "<h1>HelenOS test results</h1><ul>\n");

	fprintf(f, "<li><a href=\"result-tester.txt\">tester</a></li>\n");

	DIR *d = opendir("/test");
	if (d) {
		struct dirent *e;

		while ((e = readdir(d))) {
			if (str_lcmp(e->d_name, "test-", 5) != 0)
				continue;

			fprintf(f, "<li><a href=\"result-%s.txt\">%s"
			    "</a></li>\n", e->d_name, e->d_name);
		}
	}

	fprintf(f, "</ul></body></html>\n");
	fclose(f);
}

int main(int argc, char **argv)
{
	run_tester("/data/web/result-tester.txt");
	run_tester_fault("/tmp/tester_fault.log");
	run_pcut_tests();

	const char *fname = "/data/web/test.html";
	printf("Generating HTML report in %s\n", fname);
	gen_index(fname);
	return EXIT_SUCCESS;
}
