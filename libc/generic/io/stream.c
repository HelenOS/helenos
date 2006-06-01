/*
 * Copyright (C) 2006 Josef Cejka
 * Copyright (C) 2006 Jakub Vana
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


#include <io/io.h>
#include <io/stream.h>
#include <string.h>
#include <malloc.h>
#include <libc.h>
#include <ipc/ipc.h>
#include <ipc/ns.h>
#include <ipc/fb.h>
#include <ipc/services.h>
#include <console.h>
#include <unistd.h>

#define FDS 32

typedef struct stream_t {
	pwritefn_t w;
	preadfn_t r;
	void * param;
} stream_t;

int console_phone = -1;

stream_t streams[FDS] = {{0, 0, 0}};

/*
ssize_t write_stdout(void *param, const void * buf, size_t count);
ssize_t write_stdout(void *param, const void * buf, size_t count)
{
	return (ssize_t) __SYSCALL3(SYS_IO, 1, (sysarg_t) buf, (sysarg_t) count);
}*/

static ssize_t write_stderr(void *param, const void *buf, size_t count)
{
	return count;
	//return (ssize_t) __SYSCALL3(SYS_IO, 1, (sysarg_t) buf, (sysarg_t) count);
}

static ssize_t read_stdin(void *param, void *buf, size_t count)
{
	ipcarg_t r0,r1;
	size_t i = 0;

	while (i < count) {
		if (ipc_call_sync_2(console_phone, CONSOLE_GETCHAR, 0, 0, &r0, &r1) < 0) {
			return -1;
		}
		((char *)buf)[i++] = r0;
	}
	return i;
	//return (ssize_t) __SYSCALL3(SYS_IO, 1, (sysarg_t) buf, (sysarg_t) count);
}

static ssize_t write_stdout(void *param, const void *buf, size_t count)
{
	int i;
	ipcarg_t r0,r1;

	for (i = 0; i < count; i++)
		ipc_call_sync_2(console_phone, CONSOLE_PUTCHAR, 0, ((const char *)buf)[i], &r0, &r1);
	
	return count;
	//return (ssize_t) __SYSCALL3(SYS_IO, 1, (sysarg_t) buf, (sysarg_t) count);
}



static stream_t open_stdin(void)
{
	stream_t stream;
	int phoneid;
	int res;
	
	if (console_phone < 0) {
		while ((console_phone = ipc_connect_me_to(PHONE_NS, SERVICE_CONSOLE, 0)) < 0) {
			usleep(10000);
		}
	}
	
	stream.r = read_stdin;
	stream.param = 0;
	return stream;
}

static stream_t open_stdout(void)
{
	stream_t stream;
	int res;
	
	if (console_phone < 0) {
		while ((console_phone = ipc_connect_me_to(PHONE_NS, SERVICE_CONSOLE, 0)) < 0) {
			usleep(10000);
		}
	}
	
	stream.w = write_stdout;
	stream.param = 0;
	return stream;
}

fd_t open(const char *fname, int flags)
{
	int c = 0;
	
	while (((streams[c].w) || (streams[c].r)) && (c < FDS))
		c++;
	if (c == FDS)
		return EMFILE;
	
	if (!strcmp(fname, "stdin")) {
		streams[c] = open_stdin();
		return c;
	}
	
	if (!strcmp(fname, "stdout")) {
		//streams[c].w = write_stdout;
		//return c;
		streams[c] = open_stdout();
		return c;
	}
	
	if (!strcmp(fname, "stderr")) {
		streams[c].w = write_stderr;
		return c;
	}
}


ssize_t write(int fd, const void *buf, size_t count)
{
	if (fd < FDS)
		return streams[fd].w(streams[fd].param, buf, count);
	
	return 0;
}

ssize_t read(int fd, void *buf, size_t count)
{
	if (fd < FDS)
		return streams[fd].r(streams[fd].param, buf, count);
	
	return 0;
}

