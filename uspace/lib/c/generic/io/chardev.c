/*
 * Copyright (c) 2011 Jan Vesely
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
#include <mem.h>
#include <io/chardev.h>
#include <ipc/chardev.h>

ssize_t chardev_read(async_exch_t *exch, void *data, size_t size)
{
	if (!exch)
		return EBADMEM;
	if (size > 4 * sizeof(sysarg_t))
		return ELIMIT;

	sysarg_t message[4] = { 0 };
	const ssize_t ret = async_req_1_4(exch, CHARDEV_READ, size,
	    &message[0], &message[1], &message[2], &message[3]);
	if (ret > 0 && (size_t)ret <= size)
		memcpy(data, message, size);
	return ret;
}

ssize_t chardev_write(async_exch_t *exch, const void *data, size_t size)
{
	if (!exch)
		return EBADMEM;
	if (size > 3 * sizeof(sysarg_t))
		return ELIMIT;

	sysarg_t message[3] = { 0 };
	memcpy(message, data, size);
	return async_req_4_0(exch, CHARDEV_WRITE, size,
	    message[0], message[1], message[2]);
}
