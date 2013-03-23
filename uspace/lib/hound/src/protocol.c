/*
 * Copyright (c) 2012 Jan Vesely
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

/** @addtogroup libhound
 * @addtogroup audio
 * @{
 */
/** @file
 * Common USB functions.
 */
#include <adt/list.h>
#include <errno.h>
#include <loc.h>
#include <str.h>
#include <stdlib.h>
#include <stdio.h>
#include <libarch/types.h>

#include "protocol.h"
#include "client.h"
#include "server.h"

enum ipc_methods {
	IPC_M_HOUND_CONTEXT_REGISTER = IPC_FIRST_USER_METHOD,
	IPC_M_HOUND_CONTEXT_UNREGISTER,
	IPC_M_HOUND_STREAM_ENTER,
	IPC_M_HOUND_STREAM_EXIT,
	IPC_M_HOUND_STREAM_DRAIN,
};

/****
 * CLIENT
 ****/

const char *HOUND_SERVICE = "audio/hound";

hound_sess_t *hound_service_connect(const char *service)
{
	service_id_t id = 0;
	const int ret =
	    loc_service_get_id(service, &id, IPC_FLAG_BLOCKING);
	if (ret != EOK)
		return NULL;
	return loc_service_connect(EXCHANGE_PARALLEL, id, IPC_FLAG_BLOCKING);
}

void hound_service_disconnect(hound_sess_t *sess)
{
	if (sess)
		async_hangup(sess);
}

hound_context_id_t hound_service_register_context(hound_sess_t *sess,
    const char *name, bool record)
{
	assert(sess);
	assert(name);
	async_exch_t *exch = async_exchange_begin(sess);
	sysarg_t id;
	int ret =
	    async_req_1_1(exch, IPC_M_HOUND_CONTEXT_REGISTER, record, &id);
	if (ret == EOK)
		ret = async_data_write_start(exch, name, str_size(name));
	async_exchange_end(exch);
	return ret == EOK ? (hound_context_id_t)id : ret;
}

int hound_service_unregister_context(hound_sess_t *sess, hound_context_id_t id)
{
	assert(sess);
	async_exch_t *exch = async_exchange_begin(sess);
	const int ret =
	    async_req_1_0(exch, IPC_M_HOUND_CONTEXT_UNREGISTER, id);
	async_exchange_end(exch);
	return ret;
}

int hound_service_stream_enter(async_exch_t *exch, hound_context_id_t id,
    int flags, pcm_format_t format, size_t bsize)
{
	union {
		sysarg_t arg;
		pcm_format_t format;
	} convert = { .format = format };
	return async_req_4_0(exch, IPC_M_HOUND_STREAM_ENTER, id, flags,
	    convert.arg, bsize);
}

int hound_service_stream_exit(async_exch_t *exch)
{
	return async_req_0_0(exch, IPC_M_HOUND_STREAM_EXIT);
}

int hound_service_stream_drain(async_exch_t *exch)
{
	return async_req_0_0(exch, IPC_M_HOUND_STREAM_DRAIN);
}

int hound_service_stream_write(async_exch_t *exch, const void *data, size_t size)
{
	return async_data_write_start(exch, data, size);
}

int hound_service_stream_read(async_exch_t *exch, void *data, size_t size)
{
	return async_data_read_start(exch, data, size);
}

/****
 * SERVER
 ****/

static int hound_server_read_data(void *stream);
static int hound_server_write_data(void *stream);
static const hound_server_iface_t *server_iface;

void hound_service_set_server_iface(hound_server_iface_t *iface)
{
	server_iface = iface;
}

void hound_connection_handler(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	/* Accept connection if there is a valid iface*/
	if (server_iface) {
		async_answer_0(iid, EOK);
	} else {
		async_answer_0(iid, ENOTSUP);
		return;
	}

	while (1) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		switch (IPC_GET_IMETHOD(call)) {
		case IPC_M_HOUND_CONTEXT_REGISTER: {
			if (!server_iface || !server_iface->add_context) {
				async_answer_0(callid, ENOTSUP);
				break;
			}
			bool record = IPC_GET_ARG1(call);
			void *name;
			int ret =
			    async_data_write_accept(&name, true, 0, 0, 0, 0);
			if (ret != EOK) {
				async_answer_0(callid, ret);
				break;
			}
			hound_context_id_t id = 0;
			ret = server_iface->add_context(server_iface->server,
			    &id, name, record);
			if (ret != EOK) {
				free(name);
				async_answer_0(callid, ret);
				break;
			}
			async_answer_1(callid, EOK, id);
		}
		case IPC_M_HOUND_CONTEXT_UNREGISTER: {
			if (!server_iface || !server_iface->rem_context) {
				async_answer_0(callid, ENOTSUP);
				break;
			}
			hound_context_id_t id = IPC_GET_ARG1(call);
			const int ret =
			    server_iface->rem_context(server_iface->server, id);
			async_answer_0(callid, ret);
		}
		case IPC_M_HOUND_STREAM_ENTER: {
			if (!server_iface || !server_iface->add_stream
			    || !server_iface->is_record_context) {
				async_answer_0(callid, ENOTSUP);
				break;
			}

			hound_context_id_t id = IPC_GET_ARG1(call);
			int flags = IPC_GET_ARG2(call);
			union {
				sysarg_t arg;
				pcm_format_t format;
			} convert = { .arg = IPC_GET_ARG3(call) };
			size_t bsize = IPC_GET_ARG4(call);
			void *stream;
			int ret = server_iface->add_stream(server_iface->server,
			    id, flags, convert.format, bsize, &stream);
			if (ret != EOK) {
				async_answer_0(callid, ret);
				break;
			}
			const bool rec = server_iface->is_record_context(
			    server_iface->server, id);
			if (rec) {
				hound_server_write_data(stream);
			} else {
				hound_server_read_data(stream);
			}
			break;
		}
		case IPC_M_HOUND_STREAM_EXIT:
		case IPC_M_HOUND_STREAM_DRAIN:
			/* Stream exit/drain is only allowed in stream context*/
			async_answer_0(callid, EINVAL);
			break;
		default:
			async_answer_0(callid, ENOTSUP);
			return;
		}
	}
}

static int hound_server_read_data(void *stream)
{
	if (!server_iface || !server_iface->stream_data_write)
		return ENOTSUP;

	ipc_callid_t callid;
	ipc_call_t call;
	size_t size = 0;
	while (async_data_write_receive_call(&callid, &call, &size)) {
		char *buffer = malloc(size);
		if (!buffer) {
			async_answer_0(callid, ENOMEM);
			continue;
		}
		int ret = async_data_write_finalize(callid, buffer, size);
		if (ret == EOK) {
			ret = server_iface->stream_data_write(stream, buffer, size);
		}
		async_answer_0(callid, ret);
	}
	//TODO drain?
	const int ret = IPC_GET_IMETHOD(call) == IPC_M_HOUND_STREAM_EXIT
	    ? EOK : EINVAL;

	async_answer_0(callid, ret);
	return ret;
}

static int hound_server_write_data(void *stream)
{
	if (!server_iface || !server_iface->stream_data_read)
		return ENOTSUP;

	ipc_callid_t callid;
	ipc_call_t call;
	size_t size = 0;
	while (async_data_read_receive_call(&callid, &call, &size)) {
		char *buffer = malloc(size);
		if (!buffer) {
			async_answer_0(callid, ENOMEM);
			continue;
		}
		int ret = server_iface->stream_data_read(stream, buffer, size);
		if (ret == EOK) {
			ret = async_data_read_finalize(callid, buffer, size);
		}
		async_answer_0(callid, ret);
	}
	const int ret = IPC_GET_IMETHOD(call) == IPC_M_HOUND_STREAM_EXIT
	    ? EOK : EINVAL;

	async_answer_0(callid, ret);
	return ret;
}

/***
 * CLIENT SIDE -DEPRECATED
 ***/

typedef struct {
	data_callback_t cb;
	void *arg;
} callback_t;

hound_sess_t *hound_get_session(void)
{
	return hound_service_connect(HOUND_SERVICE);
}

void hound_release_session(hound_sess_t *sess)
{
	hound_service_disconnect(sess);
}


static int hound_register(hound_sess_t *sess, unsigned cmd, const char *name,
    unsigned channels, unsigned rate, pcm_sample_format_t format,
    data_callback_t data_callback, void *arg);
static int hound_unregister(hound_sess_t *sess, unsigned cmd, const char *name);
static int hound_connection(hound_sess_t *sess, unsigned cmd,
    const char *source, const char *sink);
static void callback_pb(ipc_callid_t iid, ipc_call_t *call, void *arg);
static void callback_rec(ipc_callid_t iid, ipc_call_t *call, void *arg);


int hound_register_playback(hound_sess_t *sess, const char *name,
    unsigned channels, unsigned rate, pcm_sample_format_t format,
    data_callback_t data_callback, void *arg)
{
	return hound_register(sess, HOUND_REGISTER_PLAYBACK, name, channels,
	    rate, format, data_callback, arg);
}
int hound_register_recording(hound_sess_t *sess, const char *name,
    unsigned channels, unsigned rate, pcm_sample_format_t format,
    data_callback_t data_callback, void *arg)
{
	return hound_register(sess, HOUND_REGISTER_RECORDING, name, channels,
	    rate, format, data_callback, arg);

}
int hound_unregister_playback(hound_sess_t *sess, const char *name)
{
	return hound_unregister(sess, HOUND_UNREGISTER_PLAYBACK, name);
}
int hound_unregister_recording(hound_sess_t *sess, const char *name)
{
	return hound_unregister(sess, HOUND_UNREGISTER_RECORDING, name);
}
int hound_create_connection(hound_sess_t *sess, const char *source, const char *sink)
{
	return hound_connection(sess, HOUND_CONNECT, source, sink);
}
int hound_destroy_connection(hound_sess_t *sess, const char *source, const char *sink)
{
	return hound_connection(sess, HOUND_DISCONNECT, source, sink);
}

static int hound_register(hound_sess_t *sess, unsigned cmd, const char *name,
    unsigned channels, unsigned rate, pcm_sample_format_t format,
    data_callback_t data_callback, void *arg)
{
	assert(cmd == HOUND_REGISTER_PLAYBACK || cmd == HOUND_REGISTER_RECORDING);
	if (!name || !data_callback || !sess)
		return EINVAL;

	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return ENOMEM;

	aid_t id = async_send_0(exch, cmd, NULL);

	int ret = async_data_write_start(exch, name, str_size(name));
	if (ret != EOK) {
		async_forget(id);
		async_exchange_end(exch);
		return ret;
	}

	callback_t *cb = malloc(sizeof(callback_t));
	if (!cb) {
		async_forget(id);
		async_exchange_end(exch);
		return ENOMEM;
	}

	cb->cb = data_callback;
	cb->arg = arg;
	async_client_conn_t callback =
	    (cmd == HOUND_REGISTER_PLAYBACK) ? callback_pb : callback_rec;

	ret = async_connect_to_me(exch, channels, rate, format, callback, cb);
	if (ret != EOK) {
		async_forget(id);
		async_exchange_end(exch);
		free(cb);
		return ret;
	}

	async_wait_for(id, (sysarg_t*)&ret);
	if (ret != EOK) {
		async_exchange_end(exch);
		free(cb);
		return ret;
	}

	async_exchange_end(exch);
	return EOK;
}

static int hound_unregister(hound_sess_t *sess, unsigned cmd, const char *name)
{
	assert(cmd == HOUND_UNREGISTER_PLAYBACK || cmd == HOUND_UNREGISTER_RECORDING);
	if (!name || !sess)
		return EINVAL;

	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return ENOMEM;
	aid_t id = async_send_0(exch, cmd, NULL);
	sysarg_t ret = async_data_write_start(exch, name, str_size(name));
	if (ret != EOK) {
		async_forget(id);
		async_exchange_end(exch);
		return ret;
	}

	async_wait_for(id, &ret);
	async_exchange_end(exch);
	return ret;
}

static int hound_connection(hound_sess_t *sess, unsigned cmd,
    const char *source, const char *sink)
{
	assert(cmd == HOUND_CONNECT || cmd == HOUND_DISCONNECT);
	if (!source || !sink || !sess)
		return EINVAL;

	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return ENOMEM;

	aid_t id = async_send_0(exch, cmd, NULL);
	sysarg_t ret = async_data_write_start(exch, source, str_size(source));
	if (ret != EOK) {
		async_forget(id);
		async_exchange_end(exch);
		return ret;
	}
	ret = async_data_write_start(exch, sink, str_size(sink));
	if (ret != EOK) {
		async_forget(id);
		async_exchange_end(exch);
		return ret;
	}
	async_wait_for(id, &ret);
	async_exchange_end(exch);
	return ret;
}
static void callback_gen(ipc_callid_t iid, ipc_call_t *call, void *arg,
    bool read)
{
	async_answer_0(iid, EOK);
	callback_t *cb = arg;
	assert(cb);
	void *buffer = NULL;
	size_t buffer_size = 0;

	bool (*receive)(ipc_callid_t *, size_t *) = read ?
	    async_data_read_receive : async_data_write_receive;

	while (1) {
		size_t size = 0;
		ipc_callid_t id = 0;
		if (!receive(&id, &size)) {
			ipc_call_t failed_call;
			async_get_call(&failed_call);
			/* Protocol error or hangup */
			if (IPC_GET_IMETHOD(failed_call) != 0)
				cb->cb(cb->arg, NULL, EIO);
			free(cb);
			return;
		}

		if (buffer_size < size) {
			buffer = realloc(buffer, size);
			if (!buffer) {
				cb->cb(cb->arg, NULL, ENOMEM);
				free(cb);
				return;
			}
			buffer_size = size;
		}
		if (read)
			cb->cb(cb->arg, buffer, size);
		const int ret = read ?
		    async_data_read_finalize(id, buffer, size):
		    async_data_write_finalize(id, buffer, size);
		if (ret != EOK) {
			cb->cb(cb->arg, NULL, ret);
			free(cb);
			return;
		}
		if (!read)
			cb->cb(cb->arg, buffer, size);
	}
}

static void callback_pb(ipc_callid_t iid, ipc_call_t *call, void *arg)
{
	callback_gen(iid, call, arg, true);
}

static void callback_rec(ipc_callid_t iid, ipc_call_t *call, void *arg)
{
	callback_gen(iid, call, arg, false);
}

/***
 * SERVER SIDE - DEPRECATED
 ***/

static const char * get_name(void);

int hound_server_register(const char *name, service_id_t *id)
{
	if (!name || !id)
		return EINVAL;

	int ret = loc_server_register(name);
	if (ret != EOK)
		return ret;

	return loc_service_register(HOUND_SERVICE, id);
}

void hound_server_unregister(service_id_t id)
{
	loc_service_unregister(id);
}

int hound_server_set_device_change_callback(dev_change_callback_t cb)
{
	return loc_register_cat_change_cb(cb);
}

int hound_server_devices_iterate(device_callback_t callback)
{
	if (!callback)
		return EINVAL;
	static bool resolved = false;
	static category_id_t cat_id = 0;

	if (!resolved) {
		const int ret = loc_category_get_id("audio-pcm", &cat_id,
		    IPC_FLAG_BLOCKING);
		if (ret != EOK)
			return ret;
		resolved = true;
	}

	service_id_t *svcs = NULL;
	size_t count = 0;
	const int ret = loc_category_get_svcs(cat_id, &svcs, &count);
	if (ret != EOK)
		return ret;

	for (unsigned i = 0; i < count; ++i) {
		char *name = NULL;
		loc_service_get_name(svcs[i], &name);
		callback(svcs[i], name);
		free(name);
	}
	free(svcs);
	return EOK;
}

int hound_server_get_register_params(const char **name, async_sess_t **sess,
    unsigned *channels, unsigned *rate, pcm_sample_format_t *format)
{
	if (!name || !sess || !channels || !rate || !format)
		return EINVAL;

	const char *n = get_name();
	if (!n)
		return ENOMEM;

	ipc_call_t call;
	ipc_callid_t callid = async_get_call(&call);

	unsigned ch = IPC_GET_ARG1(call);
	unsigned r = IPC_GET_ARG2(call);
	pcm_sample_format_t f = IPC_GET_ARG3(call);

	async_sess_t *s = async_callback_receive_start(EXCHANGE_ATOMIC, &call);
	async_answer_0(callid, s ? EOK : ENOMEM);

	*name = n;
	*sess = s;
	*channels = ch;
	*rate = r;
	*format = f;

	return ENOTSUP;
}

int hound_server_get_unregister_params(const char **name)
{
	if (!name)
		return EINVAL;
	*name = get_name();
	if (!*name)
		return ENOMEM;
	return EOK;
}

int hound_server_get_connection_params(const char **source, const char **sink)
{
	if (!source || !sink)
		return EINVAL;

	const char *source_name = get_name();
	if (!source_name)
		return ENOMEM;

	const char *sink_name = get_name();
	if (!sink_name)
		return ENOMEM;

	*source = source_name;
	*sink = sink_name;
	return EOK;
}

static const char * get_name(void)
{
	size_t size = 0;
	ipc_callid_t callid;
	async_data_write_receive(&callid, &size);
	char *buffer = malloc(size + 1);
	if (buffer) {
		async_data_write_finalize(callid, buffer, size);
		buffer[size] = 0;
	}
	return buffer;
}

/**
 * @}
 */
