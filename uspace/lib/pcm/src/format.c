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
#include <macros.h>
#include <stdio.h>

#include "format.h"

// TODO float endian?
#define float_le2host(x) (x)
#define float_be2host(x) (x)

#define host2float_le(x) (x)
#define host2float_be(x) (x)

#define from(x, type, endian) (float)(type ## _ ## endian ## 2host(x))
#define to(x, type, endian) (float)(host2 ## type ## _ ## endian(x))

/** Default linear PCM format */
const pcm_format_t AUDIO_FORMAT_DEFAULT = {
	.channels = 2,
	.sampling_rate = 44100,
	.sample_format = PCM_SAMPLE_SINT16_LE,
	};

/** Special ANY PCM format.
 * This format is used if the real format is no know or important.
 */
const pcm_format_t AUDIO_FORMAT_ANY = {
	.channels = 0,
	.sampling_rate = 0,
	.sample_format = 0,
	};

static float get_normalized_sample(const void *buffer, size_t size,
    unsigned frame, unsigned channel, const pcm_format_t *f);

/**
 * Compare PCM format attribtues.
 * @param a Format description.
 * @param b Format description.
 * @return True if a and b describe the same format, false otherwise.
 */
bool pcm_format_same(const pcm_format_t *a, const pcm_format_t* b)
{
	assert(a);
	assert(b);
	return
	    a->sampling_rate == b->sampling_rate &&
	    a->channels == b->channels &&
	    a->sample_format == b->sample_format;
}

/**
 * Fill audio buffer with silence in the specified format.
 * @param dst Destination audio buffer.
 * @param size Size of the destination audio buffer.
 * @param f Pointer to the format description.
 */
void pcm_format_silence(void *dst, size_t size, const pcm_format_t *f)
{
#define SET_NULL(type, endian, nullv) \
do { \
	type *buffer = dst; \
	const size_t sample_count = size / sizeof(type); \
	for (unsigned i = 0; i < sample_count; ++i) { \
		buffer[i] = to((type)nullv, type, endian); \
	} \
} while (0)

	switch (f->sample_format) {
	case PCM_SAMPLE_UINT8:
		SET_NULL(uint8_t, le, INT8_MIN); break;
	case PCM_SAMPLE_SINT8:
		SET_NULL(int8_t, le, 0); break;
	case PCM_SAMPLE_UINT16_LE:
		SET_NULL(uint16_t, le, INT16_MIN); break;
	case PCM_SAMPLE_SINT16_LE:
		SET_NULL(int16_t, le, 0); break;
	case PCM_SAMPLE_UINT16_BE:
		SET_NULL(uint16_t, be, INT16_MIN); break;
	case PCM_SAMPLE_SINT16_BE:
		SET_NULL(int16_t, be, 0); break;
	case PCM_SAMPLE_UINT32_LE:
		SET_NULL(uint32_t, le, INT32_MIN); break;
	case PCM_SAMPLE_SINT32_LE:
		SET_NULL(int32_t, le, 0); break;
	case PCM_SAMPLE_UINT32_BE:
		SET_NULL(uint32_t, be, INT32_MIN); break;
	case PCM_SAMPLE_SINT32_BE:
		SET_NULL(int32_t, le, 0); break;
	case PCM_SAMPLE_UINT24_32_LE:
	case PCM_SAMPLE_SINT24_32_LE:
	case PCM_SAMPLE_UINT24_32_BE:
	case PCM_SAMPLE_SINT24_32_BE:
	case PCM_SAMPLE_UINT24_LE:
	case PCM_SAMPLE_SINT24_LE:
	case PCM_SAMPLE_UINT24_BE:
	case PCM_SAMPLE_SINT24_BE:
	case PCM_SAMPLE_FLOAT32:
	default:
		break;
	}
#undef SET_NULL
}

/**
 * Mix audio data of the same format and size.
 * @param dst Destination buffer
 * @param src Source buffer
 * @param size Size of both the destination and the source buffer
 * @param f Pointer to the format descriptor.
 * @return Error code.
 */
errno_t pcm_format_mix(void *dst, const void *src, size_t size, const pcm_format_t *f)
{
	return pcm_format_convert_and_mix(dst, size, src, size, f, f);
}

/**
 * Add and mix audio data.
 * @param dst Destination audio buffer
 * @param dst_size Size of the destination buffer
 * @param src Source audio buffer
 * @param src_size Size of the source buffer.
 * @param sf Pointer to the source format descriptor.
 * @param df Pointer to the destination format descriptor.
 * @return Error code.
 *
 * Buffers must contain entire frames. Destination buffer is always filled.
 * If there are not enough data in the source buffer silent data is assumed.
 */
errno_t pcm_format_convert_and_mix(void *dst, size_t dst_size, const void *src,
    size_t src_size, const pcm_format_t *sf, const pcm_format_t *df)
{
	if (!dst || !src || !sf || !df)
		return EINVAL;
	const size_t src_frame_size = pcm_format_frame_size(sf);
	if ((src_size % src_frame_size) != 0)
		return EINVAL;

	const size_t dst_frame_size = pcm_format_frame_size(df);
	if ((dst_size % dst_frame_size) != 0)
		return EINVAL;

	/* This is so ugly it eats kittens, and puppies, and ducklings,
	 * and all little fluffy things...
	 */
#define LOOP_ADD(type, endian, low, high) \
do { \
	const unsigned frame_count = dst_size / dst_frame_size; \
	for (size_t i = 0; i < frame_count; ++i) { \
		for (unsigned j = 0; j < df->channels; ++j) { \
			const float a = \
			    get_normalized_sample(dst, dst_size, i, j, df);\
			const float b = \
			    get_normalized_sample(src, src_size, i, j, sf);\
			float c = (a + b); \
			if (c < -1.0) c = -1.0; \
			if (c > 1.0) c = 1.0; \
			c += 1.0; \
			c *= ((float)(type)high - (float)(type)low) / 2; \
			c += (float)(type)low; \
			type *dst_buf = dst; \
			const unsigned pos = i * df->channels  + j; \
			if (pos < (dst_size / sizeof(type))) \
				dst_buf[pos] = to((type)c, type, endian); \
		} \
	} \
} while (0)

	switch (df->sample_format) {
	case PCM_SAMPLE_UINT8:
		LOOP_ADD(uint8_t, le, UINT8_MIN, UINT8_MAX); break;
	case PCM_SAMPLE_SINT8:
		LOOP_ADD(uint8_t, le, INT8_MIN, INT8_MAX); break;
	case PCM_SAMPLE_UINT16_LE:
		LOOP_ADD(uint16_t, le, UINT16_MIN, UINT16_MAX); break;
	case PCM_SAMPLE_SINT16_LE:
		LOOP_ADD(int16_t, le, INT16_MIN, INT16_MAX); break;
	case PCM_SAMPLE_UINT16_BE:
		LOOP_ADD(uint16_t, be, UINT16_MIN, UINT16_MAX); break;
	case PCM_SAMPLE_SINT16_BE:
		LOOP_ADD(int16_t, be, INT16_MIN, INT16_MAX); break;
	case PCM_SAMPLE_UINT24_32_LE:
	case PCM_SAMPLE_UINT32_LE: // TODO this are not right for 24bit
		LOOP_ADD(uint32_t, le, UINT32_MIN, UINT32_MAX); break;
	case PCM_SAMPLE_SINT24_32_LE:
	case PCM_SAMPLE_SINT32_LE:
		LOOP_ADD(int32_t, le, INT32_MIN, INT32_MAX); break;
	case PCM_SAMPLE_UINT24_32_BE:
	case PCM_SAMPLE_UINT32_BE:
		LOOP_ADD(uint32_t, be, UINT32_MIN, UINT32_MAX); break;
	case PCM_SAMPLE_SINT24_32_BE:
	case PCM_SAMPLE_SINT32_BE:
		LOOP_ADD(int32_t, be, INT32_MIN, INT32_MAX); break;
	case PCM_SAMPLE_UINT24_LE:
	case PCM_SAMPLE_SINT24_LE:
	case PCM_SAMPLE_UINT24_BE:
	case PCM_SAMPLE_SINT24_BE:
	case PCM_SAMPLE_FLOAT32:
	default:
		return ENOTSUP;
	}
	return EOK;
#undef LOOP_ADD
}

/**
 * Converts all sample formats to float <-1,1>
 * @param buffer Audio data
 * @param size Size of the buffer
 * @param frame Index of the frame to read
 * @param channel Channel within the frame
 * @param f Pointer to a format descriptor
 * @return Normalized sample <-1,1>, 0.0 if the data could not be read
 */
static float get_normalized_sample(const void *buffer, size_t size,
    unsigned frame, unsigned channel, const pcm_format_t *f)
{
	assert(f);
	assert(buffer);
	if (channel >= f->channels)
		return 0.0f;
#define GET(type, endian, low, high) \
do { \
	const type *src = buffer; \
	const size_t sample_count = size / sizeof(type); \
	const size_t sample_pos = frame * f->channels + channel; \
	if (sample_pos >= sample_count) {\
		return 0.0f; \
	} \
	float sample = from(src[sample_pos], type, endian); \
	/* This makes it positive */ \
	sample -= (float)(type)low; \
	/* This makes it <0,2> */ \
	sample /= (((float)(type)high - (float)(type)low) / 2.0f); \
	return sample - 1.0f; \
} while (0)

	switch (f->sample_format) {
	case PCM_SAMPLE_UINT8:
		GET(uint8_t, le, UINT8_MIN, UINT8_MAX);
	case PCM_SAMPLE_SINT8:
		GET(int8_t, le, INT8_MIN, INT8_MAX);
	case PCM_SAMPLE_UINT16_LE:
		GET(uint16_t, le, UINT16_MIN, UINT16_MAX);
	case PCM_SAMPLE_SINT16_LE:
		GET(int16_t, le, INT16_MIN, INT16_MAX);
	case PCM_SAMPLE_UINT16_BE:
		GET(uint16_t, be, UINT16_MIN, UINT16_MAX);
	case PCM_SAMPLE_SINT16_BE:
		GET(int16_t, be, INT16_MIN, INT16_MAX);
	case PCM_SAMPLE_UINT24_32_LE:
	case PCM_SAMPLE_UINT32_LE:
		GET(uint32_t, le, UINT32_MIN, UINT32_MAX);
	case PCM_SAMPLE_SINT24_32_LE:
	case PCM_SAMPLE_SINT32_LE:
		GET(int32_t, le, INT32_MIN, INT32_MAX);
	case PCM_SAMPLE_UINT24_32_BE:
	case PCM_SAMPLE_UINT32_BE:
		GET(uint32_t, be, UINT32_MIN, UINT32_MAX);
	case PCM_SAMPLE_SINT24_32_BE:
	case PCM_SAMPLE_SINT32_BE:
		GET(int32_t, le, INT32_MIN, INT32_MAX);
	case PCM_SAMPLE_UINT24_LE:
	case PCM_SAMPLE_SINT24_LE:
	case PCM_SAMPLE_UINT24_BE:
	case PCM_SAMPLE_SINT24_BE:
	case PCM_SAMPLE_FLOAT32:
	default:
		break;
	}
	return 0;
#undef GET
}
/**
 * @}
 */
