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
#include <stdbool.h>
#include <pcm/sample_format.h>

#define DEFAULT_SINK "default"
#define DEFAULT_SOURCE "default"

typedef struct hound_context hound_context_t;
typedef struct hound_stream hound_stream_t;

hound_context_t * hound_context_create_playback(const char *name,
    unsigned channels, unsigned rate, pcm_sample_format_t format, size_t bsize);
hound_context_t * hound_context_create_capture(const char *name,
    unsigned channels, unsigned rate, pcm_sample_format_t format, size_t bsize);
void hound_context_destroy(hound_context_t *hound);

int hound_context_enable(hound_context_t *hound);
int hound_context_disable(hound_context_t *hound);

int hound_context_set_main_stream_format(hound_context_t *hound,
    unsigned channels, unsigned rate, pcm_sample_format_t format);
int hound_get_output_targets(const char **names, size_t *count);
int hound_get_input_targets(const char **names, size_t *count);

int hound_context_connect_target(hound_context_t *hound, const char* target);
int hound_context_disconnect_target(hound_context_t *hound, const char* target);

int hound_write_main_stream(hound_context_t *hound,
    const void *data, size_t size);
int hound_read_main_stream(hound_context_t *hound, void *data, size_t size);
int hound_write_replace_main_stream(hound_context_t *hound,
    const void *data, size_t size);
int hound_write_immediate_stream(hound_context_t *hound,
    const void *data, size_t size);

hound_stream_t *hound_stream_create(hound_context_t *hound, unsigned flags,
    unsigned channels, unsigned rate, pcm_sample_format_t format);
void hound_stream_destroy(hound_stream_t *stream);

int hound_stream_write(hound_stream_t *stream, const void *data, size_t size);
int hound_stream_read(hound_stream_t *stream, void *data, size_t size);





typedef async_sess_t hound_sess_t;

typedef void (*data_callback_t)(void *, void *, ssize_t);

hound_sess_t *hound_get_session(void);
void hound_release_session(hound_sess_t *sess);
int hound_register_playback(hound_sess_t *sess, const char *name,
    unsigned channels, unsigned rate, pcm_sample_format_t format,
    data_callback_t data_callback, void *arg);
int hound_register_recording(hound_sess_t *sess, const char *name,
    unsigned channels, unsigned rate, pcm_sample_format_t format,
    data_callback_t data_callback, void *arg);
int hound_unregister_playback(hound_sess_t *sess, const char *name);
int hound_unregister_recording(hound_sess_t *sess, const char *name);
int hound_create_connection(hound_sess_t *sess, const char *source, const char *sink);
int hound_destroy_connection(hound_sess_t *sess, const char *source, const char *sink);

#endif
