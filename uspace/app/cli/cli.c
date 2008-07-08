/*
 * Copyright (c) 2008 Jiri Svoboda
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

/** @addtogroup cli cli
 * @brief	Trivial command-line interface for running programs.
 * @{
 */ 
/**
 * @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <task.h>

#define LINE_BUFFER_SIZE 128
static char line_buffer[LINE_BUFFER_SIZE];

#define MAX_ARGS 16
static char *argv_buf[MAX_ARGS + 1];

static void read_line(char *buffer, int n)
{
	char c;
	int chars;

	printf("> ");

	chars = 0;
	while (chars < n - 1) {
		c = getchar();
		if (c < 0) exit(0);
		if (c == '\n') break;
		if (c == '\b') {
			if (chars > 0) {
				putchar('\b');
				--chars;
			}
			continue;
		}
		
		putchar(c);
		buffer[chars++] = c;
	}

	putchar('\n');
	buffer[chars] = '\0';
}

static void program_run(void)
{
	char *p;
	int n;

	p = line_buffer;
	n = 0;

	while (n < MAX_ARGS) {
		argv_buf[n++] = p;
		p = strchr(p, ' ');
		if (p == NULL) break;

		*p++ = '\0';
	}
	argv_buf[n] = NULL;

	printf("spawn task '%s' with %d args\n", argv_buf[0], n);
	printf("args:");
	int i;
	for (i = 0; i < n; ++i) {
		printf(" '%s'", argv_buf[i]);
	}
	printf("\n");

	task_spawn(argv_buf[0], argv_buf);
}


int main(int argc, char *argv[])
{
	printf("This is CLI\n");

	while (1) {
		read_line(line_buffer, LINE_BUFFER_SIZE);
		printf("'%s'\n", line_buffer);
		if (strcmp(line_buffer, "exit") == 0)
			break;
		
		if (line_buffer[0] != '\0')
		program_run();
	}

	printf("Bye\n");
	
	return 0;
}

/** @}
 */

