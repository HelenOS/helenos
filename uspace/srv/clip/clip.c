/*
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

#include <stdio.h>
#include <bool.h>
#include <ipc/ipc.h>
#include <async.h>
#include <ipc/services.h>
#include <ipc/clipboard.h>
#include <malloc.h>
#include <fibril_synch.h>
#include <errno.h>

#define NAME  "clip"

static char *clip_data = NULL;
static size_t clip_size = 0;
static clipboard_tag_t clip_tag = CLIPBOARD_TAG_NONE;
static FIBRIL_MUTEX_INITIALIZE(clip_mtx);

static void clip_put_data(ipc_callid_t rid, ipc_call_t *request)
{
	char *data;
	int rc;
	size_t size;
	
	switch (IPC_GET_ARG1(*request)) {
	case CLIPBOARD_TAG_NONE:
		fibril_mutex_lock(&clip_mtx);
		
		if (clip_data)
			free(clip_data);
		
		clip_data = NULL;
		clip_size = 0;
		clip_tag = CLIPBOARD_TAG_NONE;
		
		fibril_mutex_unlock(&clip_mtx);
		ipc_answer_0(rid, EOK);
		break;
	case CLIPBOARD_TAG_BLOB:
		rc = async_data_blob_receive(&data, 0, &size);
		if (rc != EOK) {
			ipc_answer_0(rid, rc);
			break;
		}
		
		fibril_mutex_lock(&clip_mtx);
		
		if (clip_data)
			free(clip_data);
		
		clip_data = data;
		clip_size = size;
		clip_tag = CLIPBOARD_TAG_BLOB;
		
		fibril_mutex_unlock(&clip_mtx);
		ipc_answer_0(rid, EOK);
		break;
	default:
		ipc_answer_0(rid, EINVAL);
	}
}

static void clip_get_data(ipc_callid_t rid, ipc_call_t *request)
{
	fibril_mutex_lock(&clip_mtx);
	
	ipc_callid_t callid;
	size_t size;
	
	/* Check for clipboard data tag compatibility */
	switch (IPC_GET_ARG1(*request)) {
	case CLIPBOARD_TAG_BLOB:
		if (!async_data_read_receive(&callid, &size)) {
			ipc_answer_0(callid, EINVAL);
			ipc_answer_0(rid, EINVAL);
			break;
		}
		
		if (clip_tag != CLIPBOARD_TAG_BLOB) {
			/* So far we only understand BLOB */
			ipc_answer_0(callid, EOVERFLOW);
			ipc_answer_0(rid, EOVERFLOW);
			break;
		}
		
		if (clip_size != size) {
			/* The client expects different size of data */
			ipc_answer_0(callid, EOVERFLOW);
			ipc_answer_0(rid, EOVERFLOW);
			break;
		}
		
		ipcarg_t retval = async_data_read_finalize(callid, clip_data, size);
		if (retval != EOK) {
			ipc_answer_0(rid, retval);
			break;
		}
		
		ipc_answer_0(rid, EOK);
	default:
		/*
		 * Sorry, we don't know how to get unknown or NONE
		 * data from the clipbard
		 */
		ipc_answer_0(rid, EINVAL);
		break;
	}
	
	fibril_mutex_unlock(&clip_mtx);
}

static void clip_content(ipc_callid_t rid, ipc_call_t *request)
{
	fibril_mutex_lock(&clip_mtx);
	
	size_t size = clip_size;
	clipboard_tag_t tag = clip_tag;
	
	fibril_mutex_unlock(&clip_mtx);
	ipc_answer_2(rid, EOK, (ipcarg_t) size, (ipcarg_t) clip_tag);
}

static void clip_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	/* Accept connection */
	ipc_answer_0(iid, EOK);
	
	bool cont = true;
	while (cont) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		switch (IPC_GET_METHOD(call)) {
		case IPC_M_PHONE_HUNGUP:
			cont = false;
			continue;
		case CLIPBOARD_PUT_DATA:
			clip_put_data(callid, &call);
			break;
		case CLIPBOARD_GET_DATA:
			clip_get_data(callid, &call);
			break;
		case CLIPBOARD_CONTENT:
			clip_content(callid, &call);
			break;
		default:
			if (!(callid & IPC_CALLID_NOTIFICATION))
				ipc_answer_0(callid, ENOENT);
		}
	}
}

int main(int argc, char *argv[])
{
	printf(NAME ": HelenOS clipboard service\n");
	
	async_set_client_connection(clip_connection);
	
	ipcarg_t phonead;
	if (ipc_connect_to_me(PHONE_NS, SERVICE_CLIPBOARD, 0, 0, &phonead) != 0) 
		return -1;
	
	printf(NAME ": Accepting connections\n");
	async_manager();
	
	/* Never reached */
	return 0;
}
