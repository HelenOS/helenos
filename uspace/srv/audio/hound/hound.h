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

/** @addtogroup audio
 * @brief HelenOS sound server
 * @{
 */
/** @file
 */

#ifndef HOUND_H_
#define HOUND_H_

#include <async.h>
#include <adt/list.h>
#include <ipc/loc.h>
#include <errno.h>
#include <fibril_synch.h>

#include "audio_source.h"
#include "audio_sink.h"
#include "audio_format.h"


typedef struct {
	fibril_mutex_t list_guard;
	list_t devices;
	list_t sources;
	list_t sinks;
} hound_t;

int hound_init(hound_t *hound);
int hound_add_device(hound_t *hound, service_id_t id, const char* name);
int hound_add_source(hound_t *hound, audio_source_t *source);
int hound_add_sink(hound_t *hound, audio_sink_t *sink);
int hound_remove_source(hound_t *hound, audio_source_t *source);
int hound_remove_sink(hound_t *hound, audio_sink_t *sink);
int hound_connect(hound_t *hound, const char* source_name, const char* sink_name);
int hound_disconnect(hound_t *hound, const char* source_name, const char* sink_name);

#endif

/**
 * @}
 */
