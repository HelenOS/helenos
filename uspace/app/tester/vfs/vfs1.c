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
		return "Mount failed.\n";

	if (mkdir("/mydir", 0) != 0)
		return "mkdir() failed.\n";
	if (!quiet)
		printf("Created directory /mydir\n");
	
	int fd0 = open("/mydir/myfile", O_CREAT);
	if (fd0 < 0)
		return "open() failed.\n";
	if (!quiet)
		printf("Created /mydir/myfile, handle=%d\n", fd0);

	ssize_t cnt;
	size_t size = sizeof(text);
	cnt = write(fd0, text, size);
	if (cnt < 0)
		return "write() failed.\n";
	if (!quiet)
		printf("Written %d btyes to handle %d.\n", cnt, fd0);
	if (lseek(fd0, 0, SEEK_SET) != 0)
		return "lseek() failed.\n";

	DIR *dirp;
	struct dirent *dp;

	dirp = opendir("/");
	if (!dirp)
		return "opendir() failed.";
	while ((dp = readdir(dirp)))
		printf("Discovered %s\n", dp->d_name);
	closedir(dirp);

	int fd1 = open("/dir1/file1", O_RDONLY);
	int fd2 = open("/dir2/file2", O_RDONLY);

	if (fd1 < 0)
		return "open() failed.\n";
	if (fd2 < 0)
		return "open() failed.\n";

	if (!quiet)
		printf("Opened file descriptors %d and %d.\n", fd1, fd2);

	char buf[10];

	cnt = read(fd0, buf, sizeof(buf));
	if (cnt < 0)
		return "read() failed.\n";

	if (!quiet)
		printf("Read %d bytes from handle %d: %.*s\n", cnt, fd0, cnt,
		    buf);

	cnt = read(fd1, buf, sizeof(buf));
	if (cnt < 0)
		return "read() failed.\n";

	if (!quiet)
		printf("Read %d bytes from handle %d: %.*s\n", cnt, fd1, cnt,
		    buf);

	return NULL;
}

