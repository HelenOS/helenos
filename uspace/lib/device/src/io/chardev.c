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

/** @addtogroup libdevice
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
errno_t chardev_open(async_sess_t *sess, chardev_t **rchardev)
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
 * @param flags @c chardev_f_nonblock to return immediately even if no
 *              bytes are available
 *
 * @return EOK on success or non-zero error code
 */
errno_t chardev_read(chardev_t *chardev, void *buf, size_t size, size_t *nread,
    chardev_flags_t flags)
{
	async_exch_t *exch = async_exchange_begin(chardev->sess);

	if (size > DATA_XFER_LIMIT) {
		/* This should not hurt anything. */
		size = DATA_XFER_LIMIT;
	}

	ipc_call_t answer;
	aid_t req = async_send_1(exch, CHARDEV_READ, flags, &answer);
	errno_t rc = async_data_read_start(exch, buf, size);
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		*nread = 0;
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);

	if (retval != EOK) {
		*nread = 0;
		return retval;
	}

	*nread = ipc_get_arg2(&answer);
	/* In case of partial success, ARG1 contains the error code */
	return (errno_t) ipc_get_arg1(&answer);

}

/** Write up to DATA_XFER_LIMIT bytes to character device.
 *
 * Write up to @a size or DATA_XFER_LIMIT bytes from @a data to character
 * device. On success EOK is returned, bytes were written and @a *nwritten
 * is set to min(@a size, DATA_XFER_LIMIT)
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
static errno_t chardev_write_once(chardev_t *chardev, const void *data,
    size_t size, size_t *nwritten)
{
	async_exch_t *exch = async_exchange_begin(chardev->sess);
	ipc_call_t answer;
	aid_t req;
	errno_t rc;

	/* Break down large transfers */
	if (size > DATA_XFER_LIMIT)
		size = DATA_XFER_LIMIT;

	req = async_send_0(exch, CHARDEV_WRITE, &answer);
	rc = async_data_write_start(exch, data, size);
	async_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		*nwritten = 0;
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);
	if (retval != EOK) {
		*nwritten = 0;
		return retval;
	}

	*nwritten = ipc_get_arg2(&answer);
	/* In case of partial success, ARG1 contains the error code */
	return (errno_t) ipc_get_arg1(&answer);
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
errno_t chardev_write(chardev_t *chardev, const void *data, size_t size,
    size_t *nwritten)
{
	size_t nw;
	size_t p;
	errno_t rc;

	p = 0;
	while (p < size) {
		rc = chardev_write_once(chardev, data + p, size - p, &nw);
		/* nw is always valid, we can have partial success */
		p += nw;

		if (rc != EOK) {
			/* We can return partial success */
			*nwritten = p;
			return rc;
		}
	}

	*nwritten = p;
	return EOK;
}

/** @}
 */
