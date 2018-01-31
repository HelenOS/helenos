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
 * @file	testread.c
 * This program checks if the file contains a pattern of increasing
 * 64 bit unsigned integers (that may overflow at some point)
 * stored in little endian byte order.
 * This is to verify that the filesystem reads
 * the files correctly.
 * If the file does not contain specified pattern, it stops
 * at the point where the file does not match and prints
 * the byte offset at which the error occured.
 * While checking the file, the program displays how many megabytes
 * it has already read, how long it took to read the last megabyte
 * and current and average read speed.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <mem.h>
#include <loc.h>
#include <byteorder.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>
#include <offset.h>
#include <str.h>

#define NAME	"testread"
#define BUFELEMS 1024
#define MBYTE (1024*1024)

static void syntax_print(void);

int main(int argc, char **argv)
{

	FILE *file;
	char *file_name;
	uint64_t *buf;
	uint64_t expected;
	aoff64_t offset;
	aoff64_t next_mark;
	aoff64_t last_mark;
	unsigned int i;
	bool check_enabled = true;
	bool progress = true;
	
	if (argc < 2) {
		printf(NAME ": Error, argument missing.\n");
		syntax_print();
		return 1;
	}
	
	/* Skip program name */
	--argc; ++argv;
	
	if (argc > 0 && str_cmp(*argv, "--no-check") == 0) {
		check_enabled = false;
		--argc; ++argv;
	}
	
	if (argc > 0 && str_cmp(*argv, "--no-progress") == 0) {
		progress = false;
		--argc; ++argv;
	}
	
	if (argc != 1) {
		printf(NAME ": Error, unexpected argument.\n");
		syntax_print();
		return 1;
	}
	
	file_name = *argv;
	
	buf = calloc(BUFELEMS, sizeof(uint64_t));
	if (buf == NULL) {
		printf("Failed allocating buffer\n");
		return 1;
	}
	
	file = fopen(file_name, "r");
	if (file == NULL) {
		printf("Failed opening file\n");
		return 1;
	}
	
	expected = 0;
	offset = 0;
	next_mark = 0;
	last_mark = 0;
	struct timeval prev_time;
	struct timeval start_time;
	gettimeofday(&start_time, NULL);
	prev_time = start_time;
	
	while (!feof(file)) {
		size_t elems = fread(buf, sizeof(uint64_t), BUFELEMS, file);
		if (ferror(file)) {
			printf("Failed reading file\n");
			fclose(file);
			free(buf);
			return 1;
		}
		
		for (i = 0; i < elems; i++) {
			if (check_enabled && uint64_t_le2host(buf[i]) != expected) {
				printf("Unexpected value at offset %" PRIuOFF64 "\n", offset);
				fclose(file);
				free(buf);
				return 2;
			}
			expected++;
			offset += sizeof(uint64_t);
		}
		
		if (progress && offset >= next_mark) {
			struct timeval cur_time;
			gettimeofday(&cur_time, NULL);
			
			uint32_t last_run = cur_time.tv_sec - prev_time.tv_sec;
			uint32_t total_time = cur_time.tv_sec - start_time.tv_sec;
			if (last_run > 0 && total_time > 0) {
				printf("%" PRIuOFF64 "M - time: %u s, "
				    "cur: %"PRIuOFF64 " B/s, avg: %" PRIuOFF64 " B/s\n",
				    offset / MBYTE, last_run,
				    (offset-last_mark)/last_run,
				    offset/total_time);
				prev_time = cur_time;
				last_mark = offset;
			}
			next_mark += MBYTE;
		}
	}
	
	struct timeval final_time;
	gettimeofday(&final_time, NULL);
	
	uint32_t total_run_time = final_time.tv_sec - start_time.tv_sec;
	if (total_run_time > 0) {
		printf("total bytes: %" PRIuOFF64 
		    ", total time: %u s, avg speed: %" PRIuOFF64 " B/s\n",
		    offset,
		    total_run_time,
		    offset/total_run_time);
	}
	
	fclose(file);
	free(buf);

	return 0;
}


static void syntax_print(void)
{
	printf("syntax: testread <filename>\n");
}

/**
 * @}
 */
