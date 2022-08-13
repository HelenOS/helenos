/*
 * SPDX-FileCopyrightText: 2009 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

#include <async.h>
#include <clipboard.h>
#include <errno.h>
#include <fibril_synch.h>
#include <ipc/clipboard.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdlib.h>
#include <str.h>

static FIBRIL_MUTEX_INITIALIZE(clip_mutex);
static async_sess_t *clip_sess = NULL;

/** Start an async exchange on the clipboard session.
 *
 * @return New exchange.
 *
 */
static async_exch_t *clip_exchange_begin(void)
{
	service_id_t sid;
	errno_t rc;

	fibril_mutex_lock(&clip_mutex);

	while (clip_sess == NULL) {
		rc = loc_service_get_id(SERVICE_NAME_CLIPBOARD, &sid,
		    IPC_FLAG_BLOCKING);
		if (rc != EOK)
			continue;

		clip_sess = loc_service_connect(sid, INTERFACE_CLIPBOARD,
		    IPC_FLAG_BLOCKING);
	}

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
 * @return Zero on success or an error code.
 *
 */
errno_t clipboard_put_str(const char *str)
{
	size_t size = str_size(str);

	if (size == 0) {
		async_exch_t *exch = clip_exchange_begin();
		errno_t rc = async_req_1_0(exch, CLIPBOARD_PUT_DATA,
		    CLIPBOARD_TAG_NONE);
		clip_exchange_end(exch);

		return (errno_t) rc;
	} else {
		async_exch_t *exch = clip_exchange_begin();
		aid_t req = async_send_1(exch, CLIPBOARD_PUT_DATA, CLIPBOARD_TAG_DATA,
		    NULL);
		errno_t rc = async_data_write_start(exch, (void *) str, size);
		clip_exchange_end(exch);

		if (rc != EOK) {
			errno_t rc_orig;
			async_wait_for(req, &rc_orig);
			if (rc_orig == EOK)
				return (errno_t) rc;
			else
				return (errno_t) rc_orig;
		}

		async_wait_for(req, &rc);

		return (errno_t) rc;
	}
}

/** Get a copy of clipboard contents.
 *
 * Returns a new string that can be deallocated with free().
 *
 * @param str Here pointer to the newly allocated string is stored.
 *
 * @return Zero on success or an error code.
 *
 */
errno_t clipboard_get_str(char **str)
{
	/* Loop until clipboard read succesful */
	while (true) {
		async_exch_t *exch = clip_exchange_begin();

		sysarg_t size;
		sysarg_t tag;
		errno_t rc = async_req_0_2(exch, CLIPBOARD_CONTENT, &size, &tag);

		clip_exchange_end(exch);

		if (rc != EOK)
			return (errno_t) rc;

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

			if ((errno_t) rc == EOVERFLOW) {
				/*
				 * The data in the clipboard has changed since
				 * the last call of CLIPBOARD_CONTENT
				 */
				break;
			}

			if (rc != EOK) {
				errno_t rc_orig;
				async_wait_for(req, &rc_orig);
				if (rc_orig == EOK)
					return (errno_t) rc;
				else
					return (errno_t) rc_orig;
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
