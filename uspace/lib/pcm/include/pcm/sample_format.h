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
 * @brief PCM sample format
 * @{
 */
/** @file
 */

#ifndef PCM_SAMPLE_FORMAT_H_
#define PCM_SAMPLE_FORMAT_H_

#include <stdbool.h>
#include <time.h>

/** Known and supported PCM sample formats */
typedef enum {
	PCM_SAMPLE_UINT8,
	PCM_SAMPLE_SINT8,
	PCM_SAMPLE_UINT16_LE,
	PCM_SAMPLE_UINT16_BE,
	PCM_SAMPLE_SINT16_LE,
	PCM_SAMPLE_SINT16_BE,
	PCM_SAMPLE_UINT24_LE,
	PCM_SAMPLE_UINT24_BE,
	PCM_SAMPLE_SINT24_LE,
	PCM_SAMPLE_SINT24_BE,
	PCM_SAMPLE_UINT24_32_LE,
	PCM_SAMPLE_UINT24_32_BE,
	PCM_SAMPLE_SINT24_32_LE,
	PCM_SAMPLE_SINT24_32_BE,
	PCM_SAMPLE_UINT32_LE,
	PCM_SAMPLE_UINT32_BE,
	PCM_SAMPLE_SINT32_LE,
	PCM_SAMPLE_SINT32_BE,
	PCM_SAMPLE_FLOAT32,
	PCM_SAMPLE_FORMAT_LAST = PCM_SAMPLE_FLOAT32,
} pcm_sample_format_t;

/**
 * Query if the format uses signed values.
 * @param format PCM sample format.
 * @return True if the format uses signed values, false otherwise.
 */
static inline bool pcm_sample_format_is_signed(pcm_sample_format_t format)
{
	switch (format) {
	case PCM_SAMPLE_SINT8:
	case PCM_SAMPLE_SINT16_LE:
	case PCM_SAMPLE_SINT16_BE:
	case PCM_SAMPLE_SINT24_LE:
	case PCM_SAMPLE_SINT24_BE:
	case PCM_SAMPLE_SINT24_32_LE:
	case PCM_SAMPLE_SINT24_32_BE:
	case PCM_SAMPLE_SINT32_LE:
	case PCM_SAMPLE_SINT32_BE:
		return true;
	case PCM_SAMPLE_UINT8:
	case PCM_SAMPLE_UINT16_LE:
	case PCM_SAMPLE_UINT16_BE:
	case PCM_SAMPLE_UINT24_LE:
	case PCM_SAMPLE_UINT24_BE:
	case PCM_SAMPLE_UINT24_32_LE:
	case PCM_SAMPLE_UINT24_32_BE:
	case PCM_SAMPLE_UINT32_LE:
	case PCM_SAMPLE_UINT32_BE:
	case PCM_SAMPLE_FLOAT32:
	default:
		return false;
	}
}

/**
 * Query byte-size of samples.
 * @param format PCM sample format.
 * @return Size in bytes of a single sample.
 */
static inline size_t pcm_sample_format_size(pcm_sample_format_t format)
{
	switch (format) {
	case PCM_SAMPLE_UINT8:
	case PCM_SAMPLE_SINT8:
		return 1;
	case PCM_SAMPLE_UINT16_LE:
	case PCM_SAMPLE_UINT16_BE:
	case PCM_SAMPLE_SINT16_LE:
	case PCM_SAMPLE_SINT16_BE:
		return 2;
	case PCM_SAMPLE_UINT24_LE:
	case PCM_SAMPLE_UINT24_BE:
	case PCM_SAMPLE_SINT24_LE:
	case PCM_SAMPLE_SINT24_BE:
		return 3;
	case PCM_SAMPLE_UINT24_32_LE:
	case PCM_SAMPLE_UINT24_32_BE:
	case PCM_SAMPLE_SINT24_32_LE:
	case PCM_SAMPLE_SINT24_32_BE:
	case PCM_SAMPLE_UINT32_LE:
	case PCM_SAMPLE_UINT32_BE:
	case PCM_SAMPLE_SINT32_LE:
	case PCM_SAMPLE_SINT32_BE:
	case PCM_SAMPLE_FLOAT32:
		return 4;
	default:
		return 0;
	}
}

/**
 * Query sie of the entire frame.
 * @param channels Number of samples in every frame.
 * @param format PCM sample format.
 * @return Size in bytes.
 */
static inline size_t pcm_sample_format_frame_size(unsigned channels,
    pcm_sample_format_t format)
{
	return pcm_sample_format_size(format) * channels;
}

/**
 * Count number of frames that fit into a buffer (even incomplete frames).
 * @param size Size of the buffer.
 * @param channels Number of samples in every frame.
 * @param format PCM sample format.
 * @return Number of frames (even incomplete).
 */
static inline size_t pcm_sample_format_size_to_frames(size_t size,
    unsigned channels, pcm_sample_format_t format)
{
	const size_t frame_size = pcm_sample_format_frame_size(channels, format);
	return (size + frame_size - 1) / frame_size;
}

/**
 * Convert byte size to time.
 * @param size Size of the buffer.
 * @param sample_rate Samples per second.
 * @param channels Number of samples in every frame.
 * @param format PCM sample format.
 * @return Number of useconds of audio data.
 */
static inline useconds_t pcm_sample_format_size_to_usec(size_t size,
    unsigned sample_rate, unsigned channels, pcm_sample_format_t format)
{
	const unsigned long long frames =
	    pcm_sample_format_size_to_frames(size, channels, format);
	return (frames * 1000000ULL) / sample_rate;
}

/**
 * Get readable name of a sample format.
 * @param format PCM sample format.
 * @return Valid string representation.
 */
static inline const char *pcm_sample_format_str(pcm_sample_format_t format)
{
	switch (format) {
	case PCM_SAMPLE_UINT8:
		return "8 bit unsigned";
	case PCM_SAMPLE_SINT8:
		return "8 bit signed";
	case PCM_SAMPLE_UINT16_LE:
		return "16 bit unsigned(LE)";
	case PCM_SAMPLE_SINT16_LE:
		return "16 bit signed(LE)";
	case PCM_SAMPLE_UINT16_BE:
		return "16 bit unsigned(BE)";
	case PCM_SAMPLE_SINT16_BE:
		return "16 bit signed(BE)";
	case PCM_SAMPLE_UINT24_LE:
		return "24 bit unsigned(LE)";
	case PCM_SAMPLE_SINT24_LE:
		return "24 bit signed(LE)";
	case PCM_SAMPLE_UINT24_BE:
		return "24 bit unsigned(BE)";
	case PCM_SAMPLE_SINT24_BE:
		return "24 bit signed(BE)";
	case PCM_SAMPLE_UINT24_32_LE:
		return "24 bit(4byte aligned) unsigned(LE)";
	case PCM_SAMPLE_UINT24_32_BE:
		return "24 bit(4byte aligned) unsigned(BE)";
	case PCM_SAMPLE_SINT24_32_LE:
		return "24 bit(4byte aligned) signed(LE)";
	case PCM_SAMPLE_SINT24_32_BE:
		return "24 bit(4byte aligned) signed(BE)";
	case PCM_SAMPLE_UINT32_LE:
		return "32 bit unsigned(LE)";
	case PCM_SAMPLE_UINT32_BE:
		return "32 bit unsigned(BE)";
	case PCM_SAMPLE_SINT32_LE:
		return "32 bit signed(LE)";
	case PCM_SAMPLE_SINT32_BE:
		return "32 bit signed(BE)";
	case PCM_SAMPLE_FLOAT32:
		return "32 bit float";
	default:
		return "Unknown sample format";
	}
}

#endif

/**
 * @}
 */

