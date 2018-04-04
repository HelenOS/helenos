/*
 * Copyright (c) 2011 Martin Sucha
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

/** @addtogroup test
 * @{
 */

/**
 * @file	bnchmark.c
 * This program measures time for various actions and writes the results
 * to a file.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <str_error.h>
#include <mem.h>
#include <loc.h>
#include <byteorder.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <str.h>

#define NAME	"bnchmark"
#define BUFSIZE 8096
#define MBYTE (1024*1024)

typedef errno_t (*measure_func_t)(void *);
typedef unsigned long umseconds_t; /* milliseconds */

static void syntax_print(void);

static errno_t measure(measure_func_t fn, void *data, umseconds_t *result)
{
	struct timeval start_time;
	gettimeofday(&start_time, NULL);

	errno_t rc = fn(data);
	if (rc != EOK) {
		fprintf(stderr, "measured function failed\n");
		return rc;
	}

	struct timeval final_time;
	gettimeofday(&final_time, NULL);

	/* Calculate time difference in milliseconds */
	*result = ((final_time.tv_usec - start_time.tv_usec) / 1000) +
	    ((final_time.tv_sec - start_time.tv_sec) * 1000);
	return EOK;
}

static errno_t sequential_read_file(void *data)
{
	char *path = (char *) data;
	char *buf = malloc(BUFSIZE);

	if (buf == NULL)
		return ENOMEM;

	FILE *file = fopen(path, "r");
	if (file == NULL) {
		fprintf(stderr, "Failed opening file: %s\n", path);
		free(buf);
		return EIO;
	}

	while (!feof(file)) {
		fread(buf, 1, BUFSIZE, file);
		if (ferror(file)) {
			fprintf(stderr, "Failed reading file\n");
			fclose(file);
			free(buf);
			return EIO;
		}
	}

	fclose(file);
	free(buf);
	return EOK;
}

static errno_t sequential_read_dir(void *data)
{
	char *path = (char *) data;

	DIR *dir = opendir(path);
	if (dir == NULL) {
		fprintf(stderr, "Failed opening directory: %s\n", path);
		return EIO;
	}

	struct dirent *dp;

	while ((dp = readdir(dir))) {
		/* Do nothing */
	}

	closedir(dir);
	return EOK;
}

int main(int argc, char **argv)
{
	errno_t rc;
	umseconds_t milliseconds_taken = 0;
	char *path = NULL;
	measure_func_t fn = NULL;
	int iteration;
	int iterations;
	char *log_str = NULL;
	char *test_type = NULL;
	char *endptr;

	if (argc < 5) {
		fprintf(stderr, NAME ": Error, argument missing.\n");
		syntax_print();
		return 1;
	}

	if (argc > 5) {
		fprintf(stderr, NAME ": Error, too many arguments.\n");
		syntax_print();
		return 1;
	}

	// Skip program name
	--argc;
	++argv;

	iterations = strtol(*argv, &endptr, 10);
	if (*endptr != '\0') {
		printf(NAME ": Error, invalid argument (iterations).\n");
		syntax_print();
		return 1;
	}

	--argc;
	++argv;
	test_type = *argv;

	--argc;
	++argv;
	log_str = *argv;

	--argc;
	++argv;
	path = *argv;

	if (str_cmp(test_type, "sequential-file-read") == 0) {
		fn = sequential_read_file;
	} else if (str_cmp(test_type, "sequential-dir-read") == 0) {
		fn = sequential_read_dir;
	} else {
		fprintf(stderr, "Error, unknown test type\n");
		syntax_print();
		return 1;
	}

	for (iteration = 0; iteration < iterations; iteration++) {
		rc = measure(fn, path, &milliseconds_taken);
		if (rc != EOK) {
			fprintf(stderr, "Error: %s\n", str_error(rc));
			return 1;
		}

		printf("%s;%s;%s;%lu;ms\n", test_type, path, log_str, milliseconds_taken);
	}

	return 0;
}


static void syntax_print(void)
{
	fprintf(stderr, "syntax: " NAME " <iterations> <test type> <log-str> <path>\n");
	fprintf(stderr, "  <iterations>    number of times to run a given test\n");
	fprintf(stderr, "  <test-type>     one of:\n");
	fprintf(stderr, "                    sequential-file-read\n");
	fprintf(stderr, "                    sequential-dir-read\n");
	fprintf(stderr, "  <log-str>       a string to attach to results\n");
	fprintf(stderr, "  <path>          file/directory to use for testing\n");
}

/**
 * @}
 */
