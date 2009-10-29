/*
 * Copyright (c) 2009 Martin Decky
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

/** @addtogroup getvc GetVC
 * @brief Console initialization task.
 * @{
 */
/**
 * @file
 */

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <task.h>
#include "version.h"

static void usage(void)
{
	printf("Usage: getvc <device> <path>\n");
}

static void reopen(FILE **stream, int fd, const char *path, int flags, const char *mode)
{
	if (fclose(*stream))
		return;
	
	*stream = NULL;
	
	int oldfd = open(path, flags);
	if (oldfd < 0)
		return;
	
	if (oldfd != fd) {
		if (dup2(oldfd, fd) != fd)
			return;
		
		if (close(oldfd))
			return;
	}
	
	*stream = fdopen(fd, mode);
}

static task_id_t spawn(char *fname)
{
	char *argv[2];
	
	argv[0] = fname;
	argv[1] = NULL;
	
	task_id_t id = task_spawn(fname, argv);
	
	if (id == 0)
		printf("Error spawning %s\n", fname);
	
	return id;
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		usage();
		return -1;
	}
	
	reopen(&stdin, 0, argv[1], O_RDONLY, "r");
	reopen(&stdout, 1, argv[1], O_WRONLY, "w");
	reopen(&stderr, 2, argv[1], O_WRONLY, "w");
	
	/*
	 * FIXME: fdopen() should actually detect that we are opening a console
	 * and it should set line-buffering mode automatically.
	 */
	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
	
	if (stdin == NULL)
		return -2;
	
	if (stdout == NULL)
		return -3;
	
	if (stderr == NULL)
		return -4;
	
	version_print(argv[1]);
	task_id_t id = spawn(argv[2]);
	
	task_exit_t texit;
	int retval;
	task_wait(id, &texit, &retval);
	
	return 0;
}

/** @}
 */
