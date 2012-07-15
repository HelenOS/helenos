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

#ifndef AUDIO_FORMAT_H_
#define AUDIO_FORMAT_H_

#include <assert.h>
#include <bool.h>
#include <pcm_sample_format.h>


typedef struct {
	unsigned channels;
	unsigned sampling_rate;
	pcm_sample_format_t sample_format;
} audio_format_t;

static const audio_format_t AUDIO_FORMAT_DEFAULT = {
	.channels = 2,
	.sampling_rate = 44100,
	.sample_format = PCM_SAMPLE_SINT16_LE,
	};

static const audio_format_t AUDIO_FORMAT_ANY = {
	.channels = 0,
	.sampling_rate = 0,
	.sample_format = 0,
	};


static inline size_t audio_format_frame_size(const audio_format_t *a)
{
	return a->channels * pcm_sample_format_size(a->sample_format);
}

bool audio_format_same(const audio_format_t *a, const audio_format_t* b);
static inline bool audio_format_is_any(const audio_format_t *f)
{
	return audio_format_same(f, &AUDIO_FORMAT_ANY);
}
int audio_format_mix(void *dst, const void *src, size_t size, const audio_format_t *f);
int audio_format_convert(audio_format_t a, void* srca, size_t sizea,
    audio_format_t b, void* srcb, size_t *sizeb);

#endif

/**
 * @}
 */
