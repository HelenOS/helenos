/*
 * Copyright (c) 2023 Jiri Svoboda
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
#include <macros.h>
#include <str.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <types/common.h>

#include "protocol.h"
#include "client.h"
#include "server.h"

enum ipc_methods {
	/** Create new context representation on the server side */
	IPC_M_HOUND_CONTEXT_REGISTER = IPC_FIRST_USER_METHOD,
	/** Release existing context representation on the server side */
	IPC_M_HOUND_CONTEXT_UNREGISTER,
	/** Request list of objects */
	IPC_M_HOUND_GET_LIST,
	/** Create new connection */
	IPC_M_HOUND_CONNECT,
	/** Destroy connection */
	IPC_M_HOUND_DISCONNECT,
	/** Switch IPC pipe to stream mode */
	IPC_M_HOUND_STREAM_ENTER,
	/** Switch IPC pipe back to general mode */
	IPC_M_HOUND_STREAM_EXIT,
	/** Wait until there is no data in the stream */
	IPC_M_HOUND_STREAM_DRAIN,
};

/** PCM format conversion helper structure */
typedef union {
	struct {
		uint16_t rate;
		uint8_t channels;
		uint8_t format;
	} __attribute__((packed)) f;
	sysarg_t arg;
} format_convert_t;

/*
 * CLIENT
 */

/** Well defined service name */
const char *HOUND_SERVICE = "audio/hound";

/** Server object */
static loc_srv_t *hound_srv;

/**
 * Start a new audio session.
 * @param service Named service typically 'HOUND_SERVICE' constant.
 * @return Valid session on success, NULL on failure.
 */
hound_sess_t *hound_service_connect(const char *service)
{
	service_id_t id = 0;
	const errno_t ret =
	    loc_service_get_id(service, &id, IPC_FLAG_BLOCKING);
	if (ret != EOK)
		return NULL;
	return loc_service_connect(id, INTERFACE_HOUND, IPC_FLAG_BLOCKING);
}

/**
 * End an existing audio session.
 * @param sess The session.
 */
void hound_service_disconnect(hound_sess_t *sess)
{
	if (sess)
		async_hangup(sess);
}

/**
 * Register a named application context to the audio server.
 * @param sess Valid audio session.
 * @param name Valid string identifier
 * @param record True if the application context wishes to receive data.
 *
 * @param[out] id  Return context ID.
 *
 * @return EOK on success, Error code on failure.
 */
errno_t hound_service_register_context(hound_sess_t *sess,
    const char *name, bool record, hound_context_id_t *id)
{
	assert(sess);
	assert(name);
	ipc_call_t call;
	async_exch_t *exch = async_exchange_begin(sess);
	aid_t mid =
	    async_send_1(exch, IPC_M_HOUND_CONTEXT_REGISTER, record, &call);
	errno_t ret = mid ? EOK : EPARTY;

	if (ret == EOK)
		ret = async_data_write_start(exch, name, str_size(name));
	else
		async_forget(mid);

	if (ret == EOK)
		async_wait_for(mid, &ret);

	async_exchange_end(exch);
	if (ret == EOK) {
		*id = (hound_context_id_t) ipc_get_arg1(&call);
	}

	return ret;
}

/**
 * Remove application context from the server's list.
 * @param sess Valid audio session.
 * @param id Valid context id.
 * @return Error code.
 */
errno_t hound_service_unregister_context(hound_sess_t *sess,
    hound_context_id_t id)
{
	assert(sess);
	async_exch_t *exch = async_exchange_begin(sess);
	const errno_t ret = async_req_1_0(exch, IPC_M_HOUND_CONTEXT_UNREGISTER,
	    cap_handle_raw(id));
	async_exchange_end(exch);
	return ret;
}

/**
 * Retrieve a list of server side actors.
 * @param[in] sess Valid audio session.
 * @param[out] ids list of string identifiers.
 * @param[out] count Number of elements int the @p ids list.
 * @param[in] flags list requirements.
 * @param[in] connection name of target actor. Used only if the list should
 *            contain connected actors.
 * @retval Error code.
 */
errno_t hound_service_get_list(hound_sess_t *sess, char ***ids, size_t *count,
    int flags, const char *connection)
{
	assert(sess);
	assert(ids);
	assert(count);

	if (connection && !(flags & HOUND_CONNECTED))
		return EINVAL;

	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return ENOMEM;

	ipc_call_t res_call;
	aid_t mid = async_send_3(exch, IPC_M_HOUND_GET_LIST, flags, *count,
	    connection != NULL, &res_call);

	errno_t ret = EOK;
	if (mid && connection)
		ret = async_data_write_start(exch, connection,
		    str_size(connection));

	if (ret == EOK)
		async_wait_for(mid, &ret);

	if (ret != EOK) {
		async_exchange_end(exch);
		return ret;
	}
	unsigned name_count = ipc_get_arg1(&res_call);

	/* Start receiving names */
	char **names = NULL;
	if (name_count) {
		size_t *sizes = calloc(name_count, sizeof(size_t));
		names = calloc(name_count, sizeof(char *));
		if (!names || !sizes)
			ret = ENOMEM;

		if (ret == EOK)
			ret = async_data_read_start(exch, sizes,
			    name_count * sizeof(size_t));
		for (unsigned i = 0; i < name_count && ret == EOK; ++i) {
			char *name = malloc(sizes[i] + 1);
			if (name) {
				memset(name, 0, sizes[i] + 1);
				ret = async_data_read_start(exch, name, sizes[i]);
				names[i] = name;
			} else {
				ret = ENOMEM;
			}
		}
		free(sizes);
	}
	async_exchange_end(exch);
	if (ret != EOK) {
		for (unsigned i = 0; i < name_count; ++i)
			free(names[i]);
		free(names);
	} else {
		*ids = names;
		*count = name_count;
	}
	return ret;
}

/**
 * Create a new connection between a source and a sink.
 * @param sess Valid audio session.
 * @param source Source name, valid string.
 * @param sink Sink name, valid string.
 * @return Error code.
 */
errno_t hound_service_connect_source_sink(hound_sess_t *sess, const char *source,
    const char *sink)
{
	assert(sess);
	assert(source);
	assert(sink);

	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return ENOMEM;
	ipc_call_t call;
	aid_t id = async_send_0(exch, IPC_M_HOUND_CONNECT, &call);
	errno_t ret = id ? EOK : EPARTY;
	if (ret == EOK)
		ret = async_data_write_start(exch, source, str_size(source));
	if (ret == EOK)
		ret = async_data_write_start(exch, sink, str_size(sink));
	async_wait_for(id, &ret);
	async_exchange_end(exch);
	return ret;
}

/**
 * Destroy an existing connection between a source and a sink.
 * @param sess Valid audio session.
 * @param source Source name, valid string.
 * @param sink Sink name, valid string.
 * @return Error code.
 */
errno_t hound_service_disconnect_source_sink(hound_sess_t *sess, const char *source,
    const char *sink)
{
	assert(sess);
	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch)
		return ENOMEM;
	ipc_call_t call;
	aid_t id = async_send_0(exch, IPC_M_HOUND_DISCONNECT, &call);
	errno_t ret = id ? EOK : EPARTY;
	if (ret == EOK)
		ret = async_data_write_start(exch, source, str_size(source));
	if (ret == EOK)
		ret = async_data_write_start(exch, sink, str_size(sink));
	async_wait_for(id, &ret);
	async_exchange_end(exch);
	return ENOTSUP;
}

/**
 * Switch IPC exchange to a STREAM mode.
 * @param exch IPC exchange.
 * @param id context id this stream should be associated with
 * @param flags set stream properties
 * @param format format of the new stream.
 * @param bsize size of the server side buffer.
 * @return Error code.
 */
errno_t hound_service_stream_enter(async_exch_t *exch, hound_context_id_t id,
    int flags, pcm_format_t format, size_t bsize)
{
	const format_convert_t c = {
		.f = {
			.channels = format.channels,
			.rate = format.sampling_rate / 100,
			.format = format.sample_format,
		}
	};

	return async_req_4_0(exch, IPC_M_HOUND_STREAM_ENTER, cap_handle_raw(id),
	    flags, c.arg, bsize);
}

/**
 * Destroy existing stream and return IPC exchange to general mode.
 * @param exch IPC exchange.
 * @return Error code.
 */
errno_t hound_service_stream_exit(async_exch_t *exch)
{
	return async_req_0_0(exch, IPC_M_HOUND_STREAM_EXIT);
}

/**
 * Wait until the server side buffer is empty.
 * @param exch IPC exchange.
 * @return Error code.
 */
errno_t hound_service_stream_drain(async_exch_t *exch)
{
	return async_req_0_0(exch, IPC_M_HOUND_STREAM_DRAIN);
}

/**
 * Write audio data to a stream.
 * @param exch IPC exchange in STREAM MODE.
 * @param data Audio data buffer.
 * @size size of the buffer
 * @return Error code.
 */
errno_t hound_service_stream_write(async_exch_t *exch, const void *data, size_t size)
{
	return async_data_write_start(exch, data, size);
}

/**
 * Read data from a stream.
 * @param exch IPC exchange in STREAM MODE.
 * @param data Audio data buffer.
 * @size size of the buffer
 * @return Error code.
 */
errno_t hound_service_stream_read(async_exch_t *exch, void *data, size_t size)
{
	return async_data_read_start(exch, data, size);
}

/*
 * SERVER
 */

static void hound_server_read_data(void *stream);
static void hound_server_write_data(void *stream);
static const hound_server_iface_t *server_iface;

/**
 * Set hound server interface implementation.
 * @param iface Initialized Hound server interface.
 */
void hound_service_set_server_iface(const hound_server_iface_t *iface)
{
	server_iface = iface;
}

/** Server side implementation of the hound protocol. IPC connection handler.
 *
 * @param icall Pointer to initial call structure.
 * @param arg   (unused)
 *
 */
void hound_connection_handler(ipc_call_t *icall, void *arg)
{
	hound_context_id_t context;
	errno_t ret;
	int flags;
	void *source;
	void *sink;

	/* Accept connection if there is a valid iface */
	if (server_iface) {
		async_accept_0(icall);
	} else {
		async_answer_0(icall, ENOTSUP);
		return;
	}

	while (true) {
		ipc_call_t call;
		async_get_call(&call);

		switch (ipc_get_imethod(&call)) {
		case IPC_M_HOUND_CONTEXT_REGISTER:
			/* check interface functions */
			if (!server_iface || !server_iface->add_context) {
				async_answer_0(&call, ENOTSUP);
				break;
			}
			bool record = ipc_get_arg1(&call);
			void *name;

			/* Get context name */
			ret = async_data_write_accept(&name, true, 0, 0, 0, 0);
			if (ret != EOK) {
				async_answer_0(&call, ret);
				break;
			}

			context = 0;
			ret = server_iface->add_context(server_iface->server,
			    &context, name, record);
			/** new context should create a copy */
			free(name);
			if (ret != EOK) {
				async_answer_0(&call, ret);
			} else {
				async_answer_1(&call, EOK, cap_handle_raw(context));
			}
			break;
		case IPC_M_HOUND_CONTEXT_UNREGISTER:
			/* check interface functions */
			if (!server_iface || !server_iface->rem_context) {
				async_answer_0(&call, ENOTSUP);
				break;
			}

			/* get id, 1st param */
			context = (hound_context_id_t) ipc_get_arg1(&call);
			ret = server_iface->rem_context(server_iface->server,
			    context);
			async_answer_0(&call, ret);
			break;
		case IPC_M_HOUND_GET_LIST:
			/* check interface functions */
			if (!server_iface || !server_iface->get_list) {
				async_answer_0(&call, ENOTSUP);
				break;
			}

			char **list = NULL;
			flags = ipc_get_arg1(&call);
			size_t count = ipc_get_arg2(&call);
			const bool conn = ipc_get_arg3(&call);
			char *conn_name = NULL;
			ret = EOK;

			/* get connected actor name if provided */
			if (conn)
				ret = async_data_write_accept(
				    (void **)&conn_name, true, 0, 0, 0, 0);

			if (ret == EOK)
				ret = server_iface->get_list(
				    server_iface->server, &list, &count,
				    conn_name, flags);
			free(conn_name);

			/* Alloc string sizes array */
			size_t *sizes = NULL;
			if (count)
				sizes = calloc(count, sizeof(size_t));
			if (count && !sizes)
				ret = ENOMEM;
			async_answer_1(&call, ret, count);

			/* We are done */
			if (count == 0 || ret != EOK)
				break;

			/* Prepare sizes table */
			for (unsigned i = 0; i < count; ++i)
				sizes[i] = str_size(list[i]);

			/* Send sizes table */
			ipc_call_t id;
			if (async_data_read_receive(&id, NULL)) {
				ret = async_data_read_finalize(&id, sizes,
				    count * sizeof(size_t));
			}
			free(sizes);

			/* Proceed to send names */
			for (unsigned i = 0; i < count; ++i) {
				size_t size = str_size(list[i]);
				ipc_call_t id;
				if (ret == EOK &&
				    async_data_read_receive(&id, NULL)) {
					ret = async_data_read_finalize(&id,
					    list[i], size);
				}
				free(list[i]);
			}
			free(list);
			break;
		case IPC_M_HOUND_CONNECT:
			/* check interface functions */
			if (!server_iface || !server_iface->connect) {
				async_answer_0(&call, ENOTSUP);
				break;
			}

			source = NULL;
			sink = NULL;

			/* read source name */
			ret = async_data_write_accept(&source, true, 0, 0, 0,
			    0);
			/* read sink name */
			if (ret == EOK)
				ret = async_data_write_accept(&sink,
				    true, 0, 0, 0, 0);

			if (ret == EOK)
				ret = server_iface->connect(
				    server_iface->server, source, sink);
			free(source);
			free(sink);
			async_answer_0(&call, ret);
			break;
		case IPC_M_HOUND_DISCONNECT:
			/* check interface functions */
			if (!server_iface || !server_iface->disconnect) {
				async_answer_0(&call, ENOTSUP);
				break;
			}

			source = NULL;
			sink = NULL;

			/* read source name */
			ret = async_data_write_accept(&source, true, 0, 0, 0,
			    0);
			/* read sink name */
			if (ret == EOK)
				ret = async_data_write_accept(&sink,
				    true, 0, 0, 0, 0);
			if (ret == EOK)
				ret = server_iface->connect(
				    server_iface->server, source, sink);
			free(source);
			free(sink);
			async_answer_0(&call, ret);
			break;
		case IPC_M_HOUND_STREAM_ENTER:
			/* check interface functions */
			if (!server_iface || !server_iface->is_record_context ||
			    !server_iface->add_stream ||
			    !server_iface->rem_stream) {
				async_answer_0(&call, ENOTSUP);
				break;
			}

			/* get parameters */
			context = (hound_context_id_t) ipc_get_arg1(&call);
			flags = ipc_get_arg2(&call);
			const format_convert_t c = { .arg = ipc_get_arg3(&call) };
			const pcm_format_t f = {
				.sampling_rate = c.f.rate * 100,
				.channels = c.f.channels,
				.sample_format = c.f.format,
			};
			size_t bsize = ipc_get_arg4(&call);

			void *stream;
			ret = server_iface->add_stream(server_iface->server,
			    context, flags, f, bsize, &stream);
			if (ret != EOK) {
				async_answer_0(&call, ret);
				break;
			}
			const bool rec = server_iface->is_record_context(
			    server_iface->server, context);
			if (rec) {
				if (server_iface->stream_data_read) {
					async_answer_0(&call, EOK);
					/* start answering read calls */
					hound_server_write_data(stream);
					server_iface->rem_stream(
					    server_iface->server, stream);
				} else {
					async_answer_0(&call, ENOTSUP);
				}
			} else {
				if (server_iface->stream_data_write) {
					async_answer_0(&call, EOK);
					/* accept write calls */
					hound_server_read_data(stream);
					server_iface->rem_stream(
					    server_iface->server, stream);
				} else {
					async_answer_0(&call, ENOTSUP);
				}
			}
			break;
		case IPC_M_HOUND_STREAM_EXIT:
		case IPC_M_HOUND_STREAM_DRAIN:
			/* Stream exit/drain is only allowed in stream context */
			async_answer_0(&call, EINVAL);
			break;
		default:
			/*
			 * In case we called async_get_call() after we had
			 * already received IPC_M_PHONE_HUNGUP deeper in the
			 * protocol handling, the capability handle will be
			 * invalid, so act carefully here.
			 */
			if (call.cap_handle != CAP_NIL)
				async_answer_0(&call, ENOTSUP);
			return;
		}
	}
}

/**
 * Read data and push it to the stream.
 * @param stream target stream, will push data there.
 */
static void hound_server_read_data(void *stream)
{
	ipc_call_t call;
	size_t size = 0;
	errno_t ret_answer = EOK;

	/* accept data write or drain */
	while (async_data_write_receive(&call, &size) ||
	    (ipc_get_imethod(&call) == IPC_M_HOUND_STREAM_DRAIN)) {
		/* check drain first */
		if (ipc_get_imethod(&call) == IPC_M_HOUND_STREAM_DRAIN) {
			errno_t ret = ENOTSUP;
			if (server_iface->drain_stream)
				ret = server_iface->drain_stream(stream);
			async_answer_0(&call, ret);
			continue;
		}

		/* there was an error last time */
		if (ret_answer != EOK) {
			async_answer_0(&call, ret_answer);
			continue;
		}

		char *buffer = malloc(size);
		if (!buffer) {
			async_answer_0(&call, ENOMEM);
			continue;
		}
		const errno_t ret = async_data_write_finalize(&call, buffer, size);
		if (ret == EOK) {
			/* push data to stream */
			ret_answer = server_iface->stream_data_write(
			    stream, buffer, size);
		}
	}
	const errno_t ret = ipc_get_imethod(&call) == IPC_M_HOUND_STREAM_EXIT ?
	    EOK : EINVAL;

	async_answer_0(&call, ret);
}

/**
 * Accept reads and pull data from the stream.
 * @param stream target stream, will pull data from there.
 */
static void hound_server_write_data(void *stream)
{

	ipc_call_t call;
	size_t size = 0;
	errno_t ret_answer = EOK;

	/* accept data read and drain */
	while (async_data_read_receive(&call, &size) ||
	    (ipc_get_imethod(&call) == IPC_M_HOUND_STREAM_DRAIN)) {
		/* drain does not make much sense but it is allowed */
		if (ipc_get_imethod(&call) == IPC_M_HOUND_STREAM_DRAIN) {
			errno_t ret = ENOTSUP;
			if (server_iface->drain_stream)
				ret = server_iface->drain_stream(stream);
			async_answer_0(&call, ret);
			continue;
		}
		/* there was an error last time */
		if (ret_answer != EOK) {
			async_answer_0(&call, ret_answer);
			continue;
		}
		char *buffer = malloc(size);
		if (!buffer) {
			async_answer_0(&call, ENOMEM);
			continue;
		}
		errno_t ret = server_iface->stream_data_read(stream, buffer, size);
		if (ret == EOK) {
			ret_answer =
			    async_data_read_finalize(&call, buffer, size);
		}
	}
	const errno_t ret = ipc_get_imethod(&call) == IPC_M_HOUND_STREAM_EXIT ?
	    EOK : EINVAL;

	async_answer_0(&call, ret);
}

/*
 * SERVER SIDE
 */

/**
 * Register new hound service to the location service.
 * @param[in] name server name
 * @param[out] id assigned service id.
 * @return Error code.
 */
errno_t hound_server_register(const char *name, service_id_t *id)
{
	errno_t rc;

	if (!name || !id)
		return EINVAL;

	if (hound_srv != NULL)
		return EBUSY;

	rc = loc_server_register(name, &hound_srv);
	if (rc != EOK)
		return rc;

	rc = loc_service_register(hound_srv, HOUND_SERVICE, id);
	if (rc != EOK) {
		loc_server_unregister(hound_srv);
		return rc;
	}

	return EOK;
}

/**
 * Unregister server from the location service.
 * @param id previously assigned service id.
 */
void hound_server_unregister(service_id_t id)
{
	loc_service_unregister(hound_srv, id);
	loc_server_unregister(hound_srv);
}

/**
 * Set callback on device category change event.
 * @param cb Callback function.
 * @return Error code.
 */
errno_t hound_server_set_device_change_callback(dev_change_callback_t cb,
    void *arg)
{
	return loc_register_cat_change_cb(cb, arg);
}

/**
 * Walk through all device in the audio-pcm category.
 * @param callback Function to call on every device.
 * @return Error code.
 */
errno_t hound_server_devices_iterate(device_callback_t callback)
{
	if (!callback)
		return EINVAL;
	static bool resolved = false;
	static category_id_t cat_id = 0;

	if (!resolved) {
		const errno_t ret = loc_category_get_id("audio-pcm", &cat_id,
		    IPC_FLAG_BLOCKING);
		if (ret != EOK)
			return ret;
		resolved = true;
	}

	service_id_t *svcs = NULL;
	size_t count = 0;
	const errno_t ret = loc_category_get_svcs(cat_id, &svcs, &count);
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
/**
 * @}
 */
