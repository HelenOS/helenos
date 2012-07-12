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

#include <assert.h>
#include <byteorder.h>
#include <errno.h>

#include "audio_format.h"

bool audio_format_same(const audio_format_t *a, const audio_format_t* b)
{
	assert(a);
	assert(b);
	return
	    a->sampling_rate == b->sampling_rate &&
	    a->channels == b->channels &&
	    a->sample_format == b->sample_format;
}

int audio_format_mix(void *dst, const void *src, size_t size, const audio_format_t *f)
{
	if (!dst || !src || !f)
		return EINVAL;
	const size_t sample_size = pcm_sample_format_size(f->sample_format);
	if (!(size / sample_size == 0))
		return EINVAL;

	/* This is so ugly it eats kittens, and puppies, and ducklings,
	 * and all little fluffy things...
	 * AND it does not check for overflows (FIXME)*/
#define LOOP_ADD(type, endian) \
do { \
	const type *src_buff = src; \
	type *dst_buff = dst; \
	for (size_t i = 0; i < size / sample_size; ++i) { \
		type a = type ## _ ## endian ##2host(dst_buff[i]); \
		type b = type ## _ ## endian ##2host(src_buff[i]); \
		type c = a + b; \
		dst_buff[i] = host2 ## type ## _ ## endian(c); \
	} \
} while (0)

	switch (f->sample_format) {
	case PCM_SAMPLE_UINT8: {
		const uint8_t *src_buff = src;
		uint8_t *dst_buff = dst;
		for (size_t i = 0; i < size; ++i )
			dst_buff[i] += src_buff[i];
		break;
		}
	case PCM_SAMPLE_SINT8: {
		const int8_t *src_buff = src;
		int8_t *dst_buff = dst;
		for (size_t i = 0; i < size; ++i)
			dst_buff[i] += src_buff[i];
		break;
		}
	case PCM_SAMPLE_UINT16_LE:
	case PCM_SAMPLE_SINT16_LE:
		LOOP_ADD(uint16_t, le); break;
	case PCM_SAMPLE_UINT16_BE:
	case PCM_SAMPLE_SINT16_BE:
		LOOP_ADD(uint16_t, be); break;
	case PCM_SAMPLE_UINT24_32_LE:
	case PCM_SAMPLE_SINT24_32_LE:
	case PCM_SAMPLE_UINT32_LE:
	case PCM_SAMPLE_SINT32_LE:
		LOOP_ADD(uint32_t, le); break;
	case PCM_SAMPLE_UINT24_32_BE:
	case PCM_SAMPLE_SINT24_32_BE:
	case PCM_SAMPLE_UINT32_BE:
	case PCM_SAMPLE_SINT32_BE:
		LOOP_ADD(uint32_t, be); break;
	case PCM_SAMPLE_UINT24_LE:
	case PCM_SAMPLE_SINT24_LE:
	case PCM_SAMPLE_UINT24_BE:
	case PCM_SAMPLE_SINT24_BE:
	case PCM_SAMPLE_FLOAT32:
	default:
		return ENOTSUP;
	}
	return EOK;
}

/**
 * @}
 */
