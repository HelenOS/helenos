/*
 * SPDX-FileCopyrightText: 2012 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup audio
 * @brief HelenOS sound server
 * @{
 */
/** @file
 */

#ifndef AUDIO_SINK_H_
#define AUDIO_SINK_H_

#include <adt/list.h>
#include <async.h>
#include <stdbool.h>
#include <fibril.h>
#include <pcm/format.h>

#include "audio_source.h"

typedef struct audio_sink audio_sink_t;

/** Audio sink abstraction structure */
struct audio_sink {
	/** Link in hound's sink list */
	link_t link;
	/** List of all related connections */
	list_t connections;
	/** Sink's name */
	char *name;
	/** Consumes data in this format */
	pcm_format_t format;
	/** Backend data */
	void *private_data;
	/** Connect/disconnect callback */
	errno_t (*connection_change)(audio_sink_t *, bool);
	/** Backend callback to check data */
	errno_t (*check_format)(audio_sink_t *);
	/** new data notifier */
	errno_t (*data_available)(audio_sink_t *);
};

/**
 * List instance helper.
 * @param l link
 * @return pointer to a sink structure, NULL on failure.
 */
static inline audio_sink_t *audio_sink_list_instance(link_t *l)
{
	return l ? list_get_instance(l, audio_sink_t, link) : NULL;
}

errno_t audio_sink_init(audio_sink_t *sink, const char *name, void *private_data,
    errno_t (*connection_change)(audio_sink_t *, bool),
    errno_t (*check_format)(audio_sink_t *), errno_t (*data_available)(audio_sink_t *),
    const pcm_format_t *f);

void audio_sink_fini(audio_sink_t *sink);
errno_t audio_sink_set_format(audio_sink_t *sink, const pcm_format_t *format);
void audio_sink_mix_inputs(audio_sink_t *sink, void *dest, size_t size);

#endif
/**
 * @}
 */
