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
 * @brief Audio PCM buffer interface.
 */

#ifndef LIBHOUND_CLIENT_H_
#define LIBHOUND_CLIENT_H_

#include <async.h>
#include <pcm/format.h>
#include <hound/protocol.h>

#define HOUND_DEFAULT_TARGET "default"
#define HOUND_ALL_TARGETS "all"

typedef struct hound_context hound_context_t;
typedef struct hound_stream hound_stream_t;

hound_context_t *hound_context_create_playback(const char *name,
    pcm_format_t format, size_t bsize);
hound_context_t *hound_context_create_capture(const char *name,
    pcm_format_t format, size_t bsize);
void hound_context_destroy(hound_context_t *hound);

errno_t hound_context_set_main_stream_params(hound_context_t *hound,
    pcm_format_t format, size_t bsize);

errno_t hound_context_get_available_targets(hound_context_t *hound,
    char ***names, size_t *count);
errno_t hound_context_get_connected_targets(hound_context_t *hound,
    char ***names, size_t *count);

errno_t hound_context_connect_target(hound_context_t *hound, const char *target);
errno_t hound_context_disconnect_target(hound_context_t *hound, const char *target);

hound_stream_t *hound_stream_create(hound_context_t *hound, unsigned flags,
    pcm_format_t format, size_t bsize);
void hound_stream_destroy(hound_stream_t *stream);

errno_t hound_stream_write(hound_stream_t *stream, const void *data, size_t size);
errno_t hound_stream_read(hound_stream_t *stream, void *data, size_t size);
errno_t hound_stream_drain(hound_stream_t *stream);

errno_t hound_write_main_stream(hound_context_t *hound,
    const void *data, size_t size);
errno_t hound_read_main_stream(hound_context_t *hound, void *data, size_t size);
errno_t hound_write_replace_main_stream(hound_context_t *hound,
    const void *data, size_t size);
errno_t hound_write_immediate(hound_context_t *hound,
    pcm_format_t format, const void *data, size_t size);

#endif
