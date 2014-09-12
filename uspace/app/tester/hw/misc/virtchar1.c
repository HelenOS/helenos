/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup tester
 * @brief Test the virtual char driver
 * @{
 */
/**
 * @file
 */

#include <inttypes.h>
#include <errno.h>
#include <str_error.h>
#include <sys/types.h>
#include <async.h>
#include <unistd.h>
#include <char_dev_iface.h>
#include <str.h>
#include <vfs/vfs.h>
#include <vfs/vfs_sess.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../../tester.h"

#define DEVICE_PATH_NORMAL  "/loc/devices/\\virt\\null\\a"

#define BUFFER_SIZE  64

static const char *test_virtchar1_internal(const char *path)
{
	TPRINTF("Opening `%s'...\n", path);
	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		TPRINTF("   ...error: %s\n", str_error(fd));
		if (fd == ENOENT) {
			TPRINTF("   (error was ENOENT: " \
			    "have you compiled test drivers?)\n");
		}
		return "Failed opening devman driver device for reading";
	}
	
	TPRINTF("   ...file handle %d\n", fd);
	
	TPRINTF(" Asking for session...\n");
	async_sess_t *sess = fd_session(EXCHANGE_SERIALIZE, fd);
	if (!sess) {
		close(fd);
		TPRINTF("   ...error: %s\n", str_error(errno));
		return "Failed to get session to device";
	}
	TPRINTF("   ...session is %p\n", sess);
	
	TPRINTF(" Will try to read...\n");
	size_t i;
	char buffer[BUFFER_SIZE];
	char_dev_read(sess, buffer, BUFFER_SIZE);
	TPRINTF(" ...verifying that we read zeroes only...\n");
	for (i = 0; i < BUFFER_SIZE; i++) {
		if (buffer[i] != 0) {
			return "Not all bytes are zeroes";
		}
	}
	TPRINTF("   ...data read okay\n");
	
	/* Clean-up. */
	TPRINTF(" Closing session and file descriptor\n");
	async_hangup(sess);
	close(fd);
	
	return NULL;
}

const char *test_virtchar1(void)
{
	const char *res = test_virtchar1_internal(DEVICE_PATH_NORMAL);
	if (res != NULL)
		return res;
	
	return NULL;
}

/** @}
 */
