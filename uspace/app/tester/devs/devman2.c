/*
 * Copyright (c) 2011 Vojtech Horky
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
 * @brief Test devman service.
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
#include <devman.h>
#include <str.h>
#include <async.h>
#include <vfs/vfs.h>
#include <vfs/vfs_sess.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../tester.h"

#define DEVICE_CLASS "test3"

const char *test_devman2(void)
{
	size_t idx = 1;
	int rc = EOK;
	const char *err_msg = NULL;
	char *path = NULL;
	while (rc == EOK) {
		rc = asprintf(&path, "/loc/class/%s\\%zu", DEVICE_CLASS, idx);
		if (rc < 0) {
			continue;
		}
		int fd = open(path, O_RDONLY);
		if (fd < 0) {
			TPRINTF("Failed opening `%s': %s.\n",
			    path, str_error(fd));
			rc = fd;
			err_msg = "Failed opening file";
			continue;
		}
		async_sess_t *sess = fd_session(EXCHANGE_SERIALIZE, fd);
		close(fd);
		if (sess == NULL) {
			TPRINTF("Failed opening phone: %s.\n", str_error(errno));
			rc = errno;
			err_msg = "Failed opening file descriptor phone";
			continue;
		}
		async_hangup(sess);
		TPRINTF("Path `%s' okay.\n", path);
		free(path);
		idx++;
		rc = EOK;
	}
	
	if (path != NULL)
		free(path);
	
	return err_msg;
}

/** @}
 */
