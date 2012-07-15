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
#include <errno.h>
#include <loc.h>
#include <str.h>
#include <stdlib.h>

#include "client.h"
#include "server.h"

const char *HOUND_SERVICE = "audio/hound";

/***
 * CLIENT SIDE
 ***/

typedef struct {
	data_callback_t cb;
	void *arg;
} callback_t;


static int hound_register(hound_sess_t *sess, unsigned cmd, const char *name,
    unsigned channels, unsigned rate, pcm_sample_format_t format,
    data_callback_t data_callback, void *arg);
static int hound_unregister(hound_sess_t *sess, unsigned cmd, const char *name);
static int hound_connection(hound_sess_t *sess, unsigned cmd,
    const char *source, const char *sink);
static void callback_pb(ipc_callid_t iid, ipc_call_t *call, void *arg);
static void callback_rec(ipc_callid_t iid, ipc_call_t *call, void *arg);


hound_sess_t *hound_get_session(void)
{
	service_id_t id = 0;
	const int ret = loc_service_get_id(HOUND_SERVICE, &id, 0);
	if (ret != EOK)
		return NULL;
	return loc_service_connect(EXCHANGE_SERIALIZE, id, 0);
}
void hound_release_session(hound_sess_t *sess)
{
	if (sess)
		async_hangup(sess);
}
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
 * SERVER SIDE
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
