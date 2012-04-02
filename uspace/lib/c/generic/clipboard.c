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
#include <ns.h>
#include <ipc/services.h>
#include <ipc/clipboard.h>
#include <fibril_synch.h>
#include <async.h>
#include <str.h>
#include <errno.h>
#include <malloc.h>

static FIBRIL_MUTEX_INITIALIZE(clip_mutex);
static async_sess_t *clip_sess = NULL;

/** Start an async exchange on the clipboard session.
 *
 * @return New exchange.
 *
 */
static async_exch_t *clip_exchange_begin(void)
{
	fibril_mutex_lock(&clip_mutex);
	
	while (clip_sess == NULL)
		clip_sess = service_connect_blocking(EXCHANGE_SERIALIZE,
		    SERVICE_CLIPBOARD, 0, 0);
	
	fibril_mutex_unlock(&clip_mutex);
	
	return async_exchange_begin(clip_sess);
}

/** Finish an async exchange on the clipboard session.
 *
 * @param exch Exchange to be finished.
 *
 */
static void clip_exchange_end(async_exch_t *exch)
{
	async_exchange_end(exch);
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
		async_exch_t *exch = clip_exchange_begin();
		sysarg_t rc = async_req_1_0(exch, CLIPBOARD_PUT_DATA,
		    CLIPBOARD_TAG_NONE);
		clip_exchange_end(exch);
		
		return (int) rc;
	} else {
		async_exch_t *exch = clip_exchange_begin();
		aid_t req = async_send_1(exch, CLIPBOARD_PUT_DATA, CLIPBOARD_TAG_DATA,
		    NULL);
		sysarg_t rc = async_data_write_start(exch, (void *) str, size);
		clip_exchange_end(exch);
		
		if (rc != EOK) {
			sysarg_t rc_orig;
			async_wait_for(req, &rc_orig);
			if (rc_orig == EOK)
				return (int) rc;
			else
				return (int) rc_orig;
		}
		
		async_wait_for(req, &rc);
		
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
		async_exch_t *exch = clip_exchange_begin();
		
		sysarg_t size;
		sysarg_t tag;
		sysarg_t rc = async_req_0_2(exch, CLIPBOARD_CONTENT, &size, &tag);
		
		clip_exchange_end(exch);
		
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
			
			exch = clip_exchange_begin();
			aid_t req = async_send_1(exch, CLIPBOARD_GET_DATA, tag, NULL);
			rc = async_data_read_start(exch, (void *) sbuf, size);
			clip_exchange_end(exch);
			
			if ((int) rc == EOVERFLOW) {
				/*
				 * The data in the clipboard has changed since
				 * the last call of CLIPBOARD_CONTENT
				 */
				break;
			}
			
			if (rc != EOK) {
				sysarg_t rc_orig;
				async_wait_for(req, &rc_orig);
				if (rc_orig == EOK)
					return (int) rc;
				else
					return (int) rc_orig;
			}
			
			async_wait_for(req, &rc);
			
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
