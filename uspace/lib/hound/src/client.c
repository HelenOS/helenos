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
#include <inttypes.h>
#include <loc.h>
#include <str.h>
#include <stdlib.h>
#include <stdio.h>
#include <types/common.h>
#include <task.h>

#include "protocol.h"
#include "client.h"

/** Stream structure */
struct hound_stream {
	/** link in context's list */
	link_t link;
	/** audio data format fo the stream */
	pcm_format_t format;
	/** IPC exchange representing the stream (in STREAM MODE) */
	async_exch_t *exch;
	/** parent context */
	hound_context_t *context;
	/** Stream flags */
	int flags;
};

/**
 * Linked list isntacen helper function.
 * @param l link
 * @return hound stream isntance.
 */
static inline hound_stream_t *hound_stream_from_link(link_t *l)
{
	return l ? list_get_instance(l, hound_stream_t, link) : NULL;
}

/** Hound client context structure */
struct hound_context {
	/** Audio session */
	hound_sess_t *session;
	/** context name, reported to the daemon */
	char *name;
	/** True if the instance is record context */
	bool record;
	/** List of associated streams */
	list_t stream_list;
	/** Main stream helper structure */
	struct {
		hound_stream_t *stream;
		pcm_format_t format;
		size_t bsize;
	} main;
	/** Assigned context id */
	hound_context_id_t id;
};

/**
 * Alloc and initialize context structure.
 * @param name Base for the real context name, will add task id.
 * @param record True if the new context should capture audio data.
 * @param format PCM format of the main pipe.
 * @param bsize Server size buffer size of the main stream.
 * @return valid pointer to initialized structure on success, NULL on failure
 */
static hound_context_t *hound_context_create(const char *name, bool record,
    pcm_format_t format, size_t bsize)
{
	hound_context_t *new_context = malloc(sizeof(hound_context_t));
	if (new_context) {
		char *cont_name;
		int ret = asprintf(&cont_name, "%" PRIu64 ":%s",
		    task_get_id(), name);
		if (ret < 0) {
			free(new_context);
			return NULL;
		}
		list_initialize(&new_context->stream_list);
		new_context->name = cont_name;
		new_context->record = record;
		new_context->session = hound_service_connect(HOUND_SERVICE);
		new_context->main.stream = NULL;
		new_context->main.format = format;
		new_context->main.bsize = bsize;
		if (!new_context->session) {
			free(new_context->name);
			free(new_context);
			return NULL;
		}
		errno_t rc = hound_service_register_context(
		    new_context->session, new_context->name, record,
		    &new_context->id);
		if (rc != EOK) {
			hound_service_disconnect(new_context->session);
			free(new_context->name);
			free(new_context);
			return NULL;
		}
	}
	return new_context;
}

/**
 * Playback context helper function.
 * @param name Base for the real context name, will add task id.
 * @param format PCM format of the main pipe.
 * @param bsize Server size buffer size of the main stream.
 * @return valid pointer to initialized structure on success, NULL on failure
 */
hound_context_t *hound_context_create_playback(const char *name,
    pcm_format_t format, size_t bsize)
{
	return hound_context_create(name, false, format, bsize);
}

/**
 * Record context helper function.
 * @param name Base for the real context name, will add task id.
 * @param format PCM format of the main pipe.
 * @param bsize Server size buffer size of the main stream.
 * @return valid pointer to initialized structure on success, NULL on failure
 */
hound_context_t *hound_context_create_capture(const char *name,
    pcm_format_t format, size_t bsize)
{
	return hound_context_create(name, true, format, bsize);
}

/**
 * Correctly dispose of the hound context structure.
 * @param hound context to remove.
 *
 * The function will destroy all associated streams first. Pointers
 * to these structures will become invalid and the function will block
 * if any of these stream needs to be drained first.
 */
void hound_context_destroy(hound_context_t *hound)
{
	assert(hound);

	while (!list_empty(&hound->stream_list)) {
		link_t *first = list_first(&hound->stream_list);
		hound_stream_t *stream = hound_stream_from_link(first);
		hound_stream_destroy(stream);
	}

	hound_service_unregister_context(hound->session, hound->id);
	hound_service_disconnect(hound->session);
	free(hound->name);
	free(hound);
}

/**
 * Get a list of possible connection targets.
 * @param[in] hound Hound context.
 * @param[out] names list of target string ids.
 * @param[out] count Number of elements in @p names list
 * @return Error code.
 *
 * The function will return deice sinks or source based on the context type.
 */
errno_t hound_context_get_available_targets(hound_context_t *hound,
    char ***names, size_t *count)
{
	assert(hound);
	assert(names);
	assert(count);
	return hound_service_get_list_all(hound->session, names, count,
	    hound->record ? HOUND_SOURCE_DEVS : HOUND_SINK_DEVS);
}

/**
 * Get a list of targets connected to the context.
 * @param[in] hound Hound context.
 * @param[out] names list of target string ids.
 * @param[out] count Number of elements in @p names list
 * @return Error code.
 */
errno_t hound_context_get_connected_targets(hound_context_t *hound,
    char ***names, size_t *count)
{
	assert(hound);
	assert(names);
	assert(count);
	return hound_service_get_list(hound->session, names, count,
	    HOUND_CONNECTED | (hound->record ?
	    HOUND_SOURCE_DEVS : HOUND_SINK_DEVS), hound->name);
}

/**
 * Create a new connection to the target.
 * @param hound Hound context.
 * @param target String identifier of the desired target.
 * @return Error code.
 *
 * The function recognizes special 'HOUND_DEFAULT_TARGET' and will
 * connect to the first possible target if it is passed this value.
 */
errno_t hound_context_connect_target(hound_context_t *hound, const char *target)
{
	assert(hound);
	assert(target);

	char **tgt = NULL;
	size_t count = 1;
	errno_t ret = EOK;
	if (str_cmp(target, HOUND_DEFAULT_TARGET) == 0) {
		ret = hound_context_get_available_targets(hound, &tgt, &count);
		if (ret != EOK)
			return ret;
		if (count == 0)
			return ENOENT;
		target = tgt[0];
	}
	//TODO handle all-targets

	if (hound->record) {
		ret = hound_service_connect_source_sink(
		    hound->session, target, hound->name);
	} else {
		ret = hound_service_connect_source_sink(
		    hound->session, hound->name, target);
	}
	if (tgt)
		free(tgt[0]);
	free(tgt);
	return ret;
}

/**
 * Destroy a connection to the target.
 * @param hound Hound context.
 * @param target String identifier of the desired target.
 * @return Error code.
 */
errno_t hound_context_disconnect_target(hound_context_t *hound, const char *target)
{
	assert(hound);
	assert(target);
	//TODO handle all-targets
	if (hound->record) {
		return hound_service_disconnect_source_sink(
		    hound->session, target, hound->name);
	} else {
		return hound_service_disconnect_source_sink(
		    hound->session, hound->name, target);
	}
}

/**
 * Create a new stream associated with the context.
 * @param hound Hound context.
 * @param flags new stream flags.
 * @param format new stream PCM format.
 * @param bzise new stream server side buffer size (in bytes)
 * @return Valid pointer to a stream instance, NULL on failure.
 */
hound_stream_t *hound_stream_create(hound_context_t *hound, unsigned flags,
    pcm_format_t format, size_t bsize)
{
	assert(hound);
	async_exch_t *stream_exch = async_exchange_begin(hound->session);
	if (!stream_exch)
		return NULL;
	hound_stream_t *new_stream = malloc(sizeof(hound_stream_t));
	if (new_stream) {
		link_initialize(&new_stream->link);
		new_stream->exch = stream_exch;
		new_stream->format = format;
		new_stream->context = hound;
		new_stream->flags = flags;
		const errno_t ret = hound_service_stream_enter(new_stream->exch,
		    hound->id, flags, format, bsize);
		if (ret != EOK) {
			async_exchange_end(new_stream->exch);
			free(new_stream);
			return NULL;
		}
		list_append(&new_stream->link, &hound->stream_list);
	}
	return new_stream;
}

/**
 * Destroy existing stream
 * @param stream The stream to destroy.
 *
 * Function will wait until the server side buffer is empty if the
 * HOUND_STREAM_DRAIN_ON_EXIT flag was set on creation.
 */
void hound_stream_destroy(hound_stream_t *stream)
{
	if (stream) {
		if (stream->flags & HOUND_STREAM_DRAIN_ON_EXIT)
			hound_service_stream_drain(stream->exch);
		hound_service_stream_exit(stream->exch);
		async_exchange_end(stream->exch);
		list_remove(&stream->link);
		free(stream);
	}
}

/**
 * Send new data to a stream.
 * @param stream The target stream
 * @param data data buffer
 * @param size size of the @p data buffer.
 * @return error code.
 */
errno_t hound_stream_write(hound_stream_t *stream, const void *data, size_t size)
{
	assert(stream);
	if (!data || size == 0)
		return EBADMEM;
	return hound_service_stream_write(stream->exch, data, size);
}

/**
 * Get data from a stream.
 * @param stream The target stream.
 * @param data data buffer.
 * @param size size of the @p data buffer.
 * @return error code.
 */
errno_t hound_stream_read(hound_stream_t *stream, void *data, size_t size)
{
	assert(stream);
	if (!data || size == 0)
		return EBADMEM;
	return hound_service_stream_read(stream->exch, data, size);
}

/**
 * Wait until the server side buffer is empty.
 * @param stream The stream that shoulod be drained.
 * @return Error code.
 */
errno_t hound_stream_drain(hound_stream_t *stream)
{
	assert(stream);
	return hound_service_stream_drain(stream->exch);
}

/**
 * Main stream getter function.
 * @param hound Houndcontext.
 * @return Valid stream pointer, NULL on failure.
 *
 * The function will create new stream, or return a pointer to the exiting one
 * if it exists.
 */
static hound_stream_t *hound_get_main_stream(hound_context_t *hound)
{
	assert(hound);
	if (!hound->main.stream)
		hound->main.stream = hound_stream_create(hound,
		    HOUND_STREAM_DRAIN_ON_EXIT, hound->main.format,
		    hound->main.bsize);
	return hound->main.stream;
}

/**
 * Send new data to the main stream.
 * @param stream The target stream
 * @param data data buffer
 * @param size size of the @p data buffer.
 * @return error code.
 */
errno_t hound_write_main_stream(hound_context_t *hound,
    const void *data, size_t size)
{
	assert(hound);
	if (hound->record)
		return EINVAL;

	hound_stream_t *mstream = hound_get_main_stream(hound);
	if (!mstream)
		return ENOMEM;
	return hound_stream_write(mstream, data, size);
}

/**
 * Get data from the main stream.
 * @param stream The target stream
 * @param data data buffer
 * @param size size of the @p data buffer.
 * @return error code.
 */
errno_t hound_read_main_stream(hound_context_t *hound, void *data, size_t size)
{
	assert(hound);
	if (!hound->record)
		return EINVAL;
	hound_stream_t *mstream = hound_get_main_stream(hound);
	if (!mstream)
		return ENOMEM;
	return hound_stream_read(mstream, data, size);
}

/**
 * Destroy the old main stream and replace it with a new one with fresh data.
 * @param hound Hound context.
 * @param data data buffer.
 * @param size size of the @p data buffer.
 * @return error code.
 *
 * NOT IMPLEMENTED
 */
errno_t hound_write_replace_main_stream(hound_context_t *hound,
    const void *data, size_t size)
{
	assert(hound);
	if (!data || size == 0)
		return EBADMEM;
	// TODO implement
	return ENOTSUP;
}

/**
 * Destroy the old main stream and replace it with a new one using new params.
 * @param hound Hound context.
 * @param channels
 * @return error code.
 *
 * NOT IMPLEMENTED
 */
errno_t hound_context_set_main_stream_params(hound_context_t *hound,
    pcm_format_t format, size_t bsize)
{
	assert(hound);
	// TODO implement
	return ENOTSUP;
}

/**
 * Write data immediately to a new stream, and wait for it to drain.
 * @param hound Hound context.
 * @param format pcm data format.
 * @param data data buffer
 * @param size @p data buffer size
 * @return Error code.
 *
 * This functnion creates a new stream writes the data, ti waits for the stream
 * to drain and destroys it before returning.
 */
errno_t hound_write_immediate(hound_context_t *hound, pcm_format_t format,
    const void *data, size_t size)
{
	assert(hound);
	if (hound->record)
		return EINVAL;
	hound_stream_t *tmpstream = hound_stream_create(hound, 0, format, size);
	if (!tmpstream)
		return ENOMEM;
	const errno_t ret = hound_stream_write(tmpstream, data, size);
	if (ret == EOK) {
		//TODO drain?
		hound_service_stream_drain(tmpstream->exch);
	}
	hound_stream_destroy(tmpstream);
	return ret;
}
/**
 * @}
 */
