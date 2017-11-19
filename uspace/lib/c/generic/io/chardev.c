/*
 * Copyright (c) 2011 Jan Vesely
 * Copyright (c) 2017 Jiri Svoboda
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

/** @addtogroup libc
 * @{
 */
/**
 * @file
 * @brief Character device client interface
 */

#include <errno.h>
#include <mem.h>
#include <io/chardev.h>
#include <ipc/chardev.h>
#include <stddef.h>
#include <stdlib.h>

/** Open character device.
 *
 * @param sess Session with the character device
 * @param rchardev Place to store pointer to the new character device structure
 *
 * @return EOK on success, ENOMEM if out of memory, EIO on I/O error
 */
int chardev_open(async_sess_t *sess, chardev_t **rchardev)
{
	chardev_t *chardev;

	chardev = calloc(1, sizeof(chardev_t));
	if (chardev == NULL)
		return ENOMEM;

	chardev->sess = sess;
	*rchardev = chardev;

	/* EIO might be used in a future implementation */
	return EOK;
}

/** Close character device.
 *
 * Frees the character device structure. The underlying session is
 * not affected.
 *
 * @param chardev Character device or @c NULL
 */
void chardev_close(chardev_t *chardev)
{
	free(chardev);
}

/** Read from character device.
 *
 * Read as much data as is available from character device up to @a size
 * bytes into @a buf. On success EOK is returned and at least one byte
 * is read (if no byte is available the function blocks). The number
 * of bytes read is stored in @a *nread.
 *
 * On error a non-zero error code is returned and @a *nread is filled with
 * the number of bytes that were successfully transferred.
 *
 * @param chardev Character device
 * @param buf Destination buffer
 * @param size Maximum number of bytes to read
 * @param nread Place to store actual number of bytes read
 *
 * @return EOK on success or non-zero error code
 */
int chardev_read(chardev_t *chardev, void *buf, size_t size, size_t *nread)
{
	if (size > 4 * sizeof(sysarg_t))
		return ELIMIT;

	async_exch_t *exch = async_exchange_begin(chardev->sess);
	sysarg_t message[4] = { 0 };
	const ssize_t ret = async_req_1_4(exch, CHARDEV_READ, size,
	    &message[0], &message[1], &message[2], &message[3]);
	async_exchange_end(exch);
	if (ret > 0 && (size_t)ret <= size)
		memcpy(buf, message, size);

	if (ret < 0) {
		*nread = 0;
		return ret;
	}

	*nread = ret;
	return EOK;
}

/** Write to character device.
 *
 * Write @a size bytes from @a data to character device. On success EOK
 * is returned, all bytes were written and @a *nwritten is set to @a size.
 *
 * On error a non-zero error code is returned and @a *nwritten is filled with
 * the number of bytes that were successfully transferred.
 *
 * @param chardev Character device
 * @param buf Destination buffer
 * @param size Maximum number of bytes to read
 * @param nwritten Place to store actual number of bytes written
 *
 * @return EOK on success or non-zero error code
 */
int chardev_write(chardev_t *chardev, const void *data, size_t size,
    size_t *nwritten)
{
	int ret;

	if (size > 3 * sizeof(sysarg_t))
		return ELIMIT;

	async_exch_t *exch = async_exchange_begin(chardev->sess);
	sysarg_t message[3] = { 0 };
	memcpy(message, data, size);
	ret = async_req_4_0(exch, CHARDEV_WRITE, size,
	    message[0], message[1], message[2]);
	async_exchange_end(exch);

	if (ret < 0) {
		*nwritten = 0;
		return ret;
	}

	*nwritten = ret;
	return EOK;
}

/** @}
 */
