/*
 * SPDX-FileCopyrightText: 2013 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @addtogroup audio
 * @brief HelenOS sound server.
 * @{
 */
/** @file
 */

#include <errno.h>
#include <hound/protocol.h>
#include <inttypes.h>

#include "hound.h"
#include "hound_ctx.h"
#include "log.h"

static errno_t iface_add_context(void *server, hound_context_id_t *id,
    const char *name, bool record)
{
	assert(server);
	assert(id);
	assert(name);

	hound_ctx_t *ctx = record ? hound_record_ctx_get(name) :
	    hound_playback_ctx_get(name);
	if (!ctx)
		return ENOMEM;

	const errno_t ret = hound_add_ctx(server, ctx);
	if (ret != EOK)
		hound_ctx_destroy(ctx);
	else
		*id = hound_ctx_get_id(ctx);
	return ret;
}

static errno_t iface_rem_context(void *server, hound_context_id_t id)
{
	assert(server);
	hound_ctx_t *ctx = hound_get_ctx_by_id(server, id);
	if (!ctx)
		return EINVAL;
	const errno_t ret = hound_remove_ctx(server, ctx);
	if (ret == EOK) {
		hound_ctx_destroy(ctx);
		log_info("%s: %p, %p", __FUNCTION__, server, id);
	}
	return ret;
}

static bool iface_is_record_context(void *server, hound_context_id_t id)
{
	assert(server);
	hound_ctx_t *ctx = hound_get_ctx_by_id(server, id);
	if (!ctx)
		return false;
	return hound_ctx_is_record(ctx);
}

static errno_t iface_get_list(void *server, char ***list, size_t *size,
    const char *connection, int flags)
{
	log_info("%s: %p, %zu, %s, %#x\n", __FUNCTION__, server, *size,
	    connection, flags);
	if ((flags & (HOUND_SINK_DEVS | HOUND_SINK_APPS)) != 0)
		return hound_list_sinks(server, list, size);
	if ((flags & (HOUND_SOURCE_DEVS | HOUND_SOURCE_APPS)) != 0)
		return hound_list_sources(server, list, size);
	return ENOTSUP;
}

static errno_t iface_connect(void *server, const char *source, const char *sink)
{
	log_info("%s: %p, %s -> %s", __FUNCTION__, server, source, sink);
	return hound_connect(server, source, sink);
}

static errno_t iface_disconnect(void *server, const char *source, const char *sink)
{
	log_info("%s: %p, %s -> %s", __FUNCTION__, server, source, sink);
	return hound_disconnect(server, source, sink);
}

static errno_t iface_add_stream(void *server, hound_context_id_t id, int flags,
    pcm_format_t format, size_t size, void **data)
{
	assert(data);
	assert(server);

	log_verbose("%s: %p, %p %x ch:%u r:%u f:%s", __FUNCTION__, server, id,
	    flags, format.channels, format.sampling_rate,
	    pcm_sample_format_str(format.sample_format));
	hound_ctx_t *ctx = hound_get_ctx_by_id(server, id);
	if (!ctx)
		return ENOENT;
	hound_ctx_stream_t *stream =
	    hound_ctx_create_stream(ctx, flags, format, size);
	if (!stream)
		return ENOMEM;
	*data = stream;
	return EOK;
}

static errno_t iface_rem_stream(void *server, void *stream)
{
	hound_ctx_destroy_stream(stream);
	return EOK;
}

static errno_t iface_drain_stream(void *stream)
{
	hound_ctx_stream_drain(stream);
	return EOK;
}

static errno_t iface_stream_data_read(void *stream, void *buffer, size_t size)
{
	return hound_ctx_stream_read(stream, buffer, size);
}

static errno_t iface_stream_data_write(void *stream, void *buffer, size_t size)
{
	return hound_ctx_stream_write(stream, buffer, size);
}

hound_server_iface_t hound_iface = {
	.add_context = iface_add_context,
	.rem_context = iface_rem_context,
	.is_record_context = iface_is_record_context,
	.get_list = iface_get_list,
	.connect = iface_connect,
	.disconnect = iface_disconnect,
	.add_stream = iface_add_stream,
	.rem_stream = iface_rem_stream,
	.drain_stream = iface_drain_stream,
	.stream_data_write = iface_stream_data_write,
	.stream_data_read = iface_stream_data_read,
	.server = NULL,
};
