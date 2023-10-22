/*
 * Copyright (c) 2023 Jiri Svoboda
 * Copyright (c) 2009 Martin Decky
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

#include <async.h>
#include <errno.h>
#include <str_error.h>
#include <fibril_synch.h>
#include <ipc/services.h>
#include <ipc/clipboard.h>
#include <loc.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <task.h>

#define NAME  "clipboard"

static char *clip_data = NULL;
static size_t clip_size = 0;
static clipboard_tag_t clip_tag = CLIPBOARD_TAG_NONE;
static FIBRIL_MUTEX_INITIALIZE(clip_mtx);
static service_id_t svc_id;

static void clip_put_data(ipc_call_t *req)
{
	char *data;
	errno_t rc;
	size_t size;

	switch (ipc_get_arg1(req)) {
	case CLIPBOARD_TAG_NONE:
		fibril_mutex_lock(&clip_mtx);

		if (clip_data)
			free(clip_data);

		clip_data = NULL;
		clip_size = 0;
		clip_tag = CLIPBOARD_TAG_NONE;

		fibril_mutex_unlock(&clip_mtx);
		async_answer_0(req, EOK);
		break;
	case CLIPBOARD_TAG_DATA:
		rc = async_data_write_accept((void **) &data, false, 0, 0, 0, &size);
		if (rc != EOK) {
			async_answer_0(req, rc);
			break;
		}

		fibril_mutex_lock(&clip_mtx);

		if (clip_data)
			free(clip_data);

		clip_data = data;
		clip_size = size;
		clip_tag = CLIPBOARD_TAG_DATA;

		fibril_mutex_unlock(&clip_mtx);
		async_answer_0(req, EOK);
		break;
	default:
		async_answer_0(req, EINVAL);
	}
}

static void clip_get_data(ipc_call_t *req)
{
	fibril_mutex_lock(&clip_mtx);

	ipc_call_t call;
	size_t size;

	/* Check for clipboard data tag compatibility */
	switch (ipc_get_arg1(req)) {
	case CLIPBOARD_TAG_DATA:
		if (!async_data_read_receive(&call, &size)) {
			async_answer_0(&call, EINVAL);
			async_answer_0(req, EINVAL);
			break;
		}

		if (clip_tag != CLIPBOARD_TAG_DATA) {
			/* So far we only understand binary data */
			async_answer_0(&call, EOVERFLOW);
			async_answer_0(req, EOVERFLOW);
			break;
		}

		if (clip_size != size) {
			/* The client expects different size of data */
			async_answer_0(&call, EOVERFLOW);
			async_answer_0(req, EOVERFLOW);
			break;
		}

		errno_t retval = async_data_read_finalize(&call, clip_data, size);
		if (retval != EOK) {
			async_answer_0(req, retval);
			break;
		}

		async_answer_0(req, EOK);
		break;
	default:
		/*
		 * Sorry, we don't know how to get unknown or NONE
		 * data from the clipbard
		 */
		async_answer_0(req, EINVAL);
		break;
	}

	fibril_mutex_unlock(&clip_mtx);
}

static void clip_content(ipc_call_t *req)
{
	fibril_mutex_lock(&clip_mtx);

	size_t size = clip_size;
	clipboard_tag_t tag = clip_tag;

	fibril_mutex_unlock(&clip_mtx);
	async_answer_2(req, EOK, (sysarg_t) size, (sysarg_t) tag);
}

static void clip_connection(ipc_call_t *icall, void *arg)
{
	/* Accept connection */
	async_accept_0(icall);

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		if (!ipc_get_imethod(&call)) {
			async_answer_0(&call, EOK);
			break;
		}

		switch (ipc_get_imethod(&call)) {
		case CLIPBOARD_PUT_DATA:
			clip_put_data(&call);
			break;
		case CLIPBOARD_GET_DATA:
			clip_get_data(&call);
			break;
		case CLIPBOARD_CONTENT:
			clip_content(&call);
			break;
		default:
			async_answer_0(&call, ENOENT);
		}
	}
}

int main(int argc, char *argv[])
{
	errno_t rc;
	loc_srv_t *srv;

	printf("%s: HelenOS clipboard service\n", NAME);
	async_set_fallback_port_handler(clip_connection, NULL);

	rc = loc_server_register(NAME, &srv);
	if (rc != EOK) {
		printf("%s: Failed registering server: %s\n", NAME, str_error(rc));
		return rc;
	}

	rc = loc_service_register(srv, SERVICE_NAME_CLIPBOARD, &svc_id);
	if (rc != EOK) {
		loc_server_unregister(srv);

		printf("%s: Failed registering service : %s\n", NAME, str_error(rc));
		return rc;
	}

	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();

	/* Never reached */
	return 0;
}
