/*
 * Copyright (c) 2008 Jakub Jermar
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
#include <string.h>
#include <vfs.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../tester.h"

char text[] = "O xein', angellein Lakedaimoniois hoti teide "
	"keimetha tois keinon rhemasi peithomenoi.";

char *test_vfs1(bool quiet)
{
	if (mount("tmpfs", "/", "nulldev0") != EOK)
		return "mount() failed.\n";
	if (!quiet)
		printf("mounted tmpfs on /.\n");

	if (mkdir("/mydir", 0) != 0)
		return "mkdir() failed.\n";
	if (!quiet)
		printf("created directory /mydir\n");
	
	int fd0 = open("/mydir/myfile", O_CREAT);
	if (fd0 < 0)
		return "open() failed.\n";
	if (!quiet)
		printf("created file /mydir/myfile, fd=%d\n", fd0);

	ssize_t cnt;
	size_t size = sizeof(text);
	cnt = write(fd0, text, size);
	if (cnt < 0)
		return "write() failed.\n";
	if (!quiet)
		printf("written %d bytes, fd=%d\n", cnt, fd0);
	if (lseek(fd0, 0, SEEK_SET) != 0)
		return "lseek() failed.\n";
	if (!quiet)
		printf("sought to position 0, fd=%d\n", fd0);

	char buf[10];

	cnt = read(fd0, buf, sizeof(buf));
	if (cnt < 0)
		return "read() failed.\n";

	if (!quiet)
		printf("read %d bytes: \"%.*s\", fd=%d\n", cnt, cnt, buf, fd0);

	DIR *dirp;
	struct dirent *dp;

	dirp = opendir("/");
	if (!dirp)
		return "opendir() failed.";
	while ((dp = readdir(dirp)))
		printf("discovered node %s in /\n", dp->d_name);
	closedir(dirp);

	return NULL;
}

