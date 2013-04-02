/*
 * Copyright (c) 2013 Jan Vesely
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

/**
 * @addtogroup audio
 * @brief HelenOS sound server.
 * @{
 */
/** @file
 */

#include <errno.h>
#include <hound/protocol.h>
#include <malloc.h>

#include "hound.h"
#include "hound_ctx.h"
#include "log.h"

static int iface_add_context(void *server, hound_context_id_t *id,
    const char *name, bool record)
{
	assert(server);
	assert(id);
	assert(name);

	hound_ctx_t *ctx = record ? hound_record_ctx_get(name) :
	    hound_playback_ctx_get(name);
	if (!ctx)
		return ENOMEM;

	const int ret = hound_add_ctx(server, ctx);
	if (ret != EOK)
		hound_ctx_destroy(ctx);
	else
		*id = hound_ctx_get_id(ctx);
	return ret;
}

static int iface_rem_context(void *server, hound_context_id_t id)
{
	assert(server);
	hound_ctx_t *ctx = hound_get_ctx_by_id(server, id);
	if (!ctx)
		return EINVAL;
	hound_remove_ctx(server, ctx);
	hound_ctx_destroy(ctx);
	log_info("%s: %p, %d", __FUNCTION__, server, id);
	return EOK;
}

static bool iface_is_record_context(void *server, hound_context_id_t id)
{
	assert(server);
	hound_ctx_t *ctx = hound_get_ctx_by_id(server, id);
	if (!ctx)
		return false;
	log_info("%s: %p, %d", __FUNCTION__, server, id);
	return hound_ctx_is_record(ctx);
}

static int iface_get_list(void *server, const char ***list, size_t *size,
    const char *connection, int flags)
{
	log_info("%s: %p, %zu, %s, %u", __FUNCTION__, server, *size, connection, flags);
	*list = malloc(sizeof(char *));
	**list = str_dup("FOO SINK");
	*size = 1;
	return EOK;
}

static int iface_connect(void *server, const char *source, const char *sink)
{
	log_info("%s: %p, %s -> %s", __FUNCTION__, server, source, sink);
	return ENOTSUP;
}

static int iface_disconnect(void *server, const char *source, const char *sink)
{
	log_info("%s: %p, %s -> %s", __FUNCTION__, server, source, sink);
	return ENOTSUP;
}

static int iface_add_stream(void *server, hound_context_id_t id, int flags,
    pcm_format_t format, size_t size, void **data)
{
	log_info("%s: %p, %d %x ch:%u r:%u f:%s", __FUNCTION__, server, id,
	    flags, format.channels, format.sampling_rate,
	    pcm_sample_format_str(format.sample_format));
	*data = (void*)"TEST_STREAM";
	return ENOTSUP;
}

static int iface_rem_stream(void *server, void *stream)
{
	log_info("%s: %p, %s", __FUNCTION__, server, (char *)stream);
	return ENOTSUP;
}

static int iface_stream_data_read(void *stream, void *buffer, size_t size)
{
	log_info("%s: %s, %zu", __FUNCTION__, (char *)stream, size);
	return ENOTSUP;
}

static int iface_stream_data_write(void *stream, const void *buffer, size_t size)
{
	log_info("%s: %s, %zu", __FUNCTION__, (char *)stream, size);
	return ENOTSUP;
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
	.stream_data_write = iface_stream_data_write,
	.stream_data_read = iface_stream_data_read,
	.server = NULL,
};
