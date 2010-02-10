/*
 * Copyright (c) 2009 Jiri Svoboda
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
/** @file
 * @brief System clipboard API.
 *
 * The clipboard data is managed by the clipboard service and it is shared by
 * the entire system.
 *
 */

#include <clipboard.h>
#include <ipc/services.h>
#include <ipc/clipboard.h>
#include <async.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>

static int clip_phone = -1;

/** Connect to clipboard server
 *
 */
static void clip_connect(void)
{
	while (clip_phone < 0)
		clip_phone = ipc_connect_me_to_blocking(PHONE_NS, SERVICE_CLIPBOARD, 0, 0);
}

/** Copy string to clipboard.
 *
 * Sets the clipboard contents to @a str. Passing an empty string or NULL
 * makes the clipboard empty.
 *
 * @param str String to put to clipboard or NULL.
 *
 * @return Zero on success or negative error code.
 *
 */
int clipboard_put_str(const char *str)
{
	size_t size = str_size(str);
	
	if (size == 0) {
		async_serialize_start();
		clip_connect();
		
		ipcarg_t rc = async_req_1_0(clip_phone, CLIPBOARD_PUT_DATA, CLIPBOARD_TAG_NONE);
		
		async_serialize_end();
		
		return (int) rc;
	} else {
		async_serialize_start();
		clip_connect();
		
		aid_t req = async_send_1(clip_phone, CLIPBOARD_PUT_DATA, CLIPBOARD_TAG_DATA, NULL);
		ipcarg_t rc = async_data_write_start(clip_phone, (void *) str, size);
		if (rc != EOK) {
			ipcarg_t rc_orig;
			async_wait_for(req, &rc_orig);
			async_serialize_end();
			if (rc_orig == EOK)
				return (int) rc;
			else
				return (int) rc_orig;
		}
		
		async_wait_for(req, &rc);
		async_serialize_end();
		
		return (int) rc;
	}
}

/** Get a copy of clipboard contents.
 *
 * Returns a new string that can be deallocated with free().
 *
 * @param str Here pointer to the newly allocated string is stored.
 *
 * @return Zero on success or negative error code.
 *
 */
int clipboard_get_str(char **str)
{
	/* Loop until clipboard read succesful */
	while (true) {
		async_serialize_start();
		clip_connect();
		
		ipcarg_t size;
		ipcarg_t tag;
		ipcarg_t rc = async_req_0_2(clip_phone, CLIPBOARD_CONTENT, &size, &tag);
		
		async_serialize_end();
		
		if (rc != EOK)
			return (int) rc;
		
		char *sbuf;
		
		switch (tag) {
		case CLIPBOARD_TAG_NONE:
			sbuf = malloc(1);
			if (sbuf == NULL)
				return ENOMEM;
			
			sbuf[0] = 0;
			*str = sbuf;
			return EOK;
		case CLIPBOARD_TAG_DATA:
			sbuf = malloc(size + 1);
			if (sbuf == NULL)
				return ENOMEM;
			
			async_serialize_start();
			
			aid_t req = async_send_1(clip_phone, CLIPBOARD_GET_DATA, tag, NULL);
			rc = async_data_read_start(clip_phone, (void *) sbuf, size);
			if ((int) rc == EOVERFLOW) {
				/*
				 * The data in the clipboard has changed since
				 * the last call of CLIPBOARD_CONTENT
				 */
				async_serialize_end();
				break;
			}
			
			if (rc != EOK) {
				ipcarg_t rc_orig;
				async_wait_for(req, &rc_orig);
				async_serialize_end();
				if (rc_orig == EOK)
					return (int) rc;
				else
					return (int) rc_orig;
			}
			
			async_wait_for(req, &rc);
			async_serialize_end();
			
			if (rc == EOK) {
				sbuf[size] = 0;
				*str = sbuf;
			}
			
			return rc;
		default:
			return EINVAL;
		}
	}
}

/** @}
 */
