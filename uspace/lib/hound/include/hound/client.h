/*
 * SPDX-FileCopyrightText: 2012 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
