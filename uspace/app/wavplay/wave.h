/*
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup dplay
 * @{
 */
/** @file
 * @brief .wav file format.
 */
#ifndef WAVE_H
#define WAVE_H

#include <stdint.h>
#include <pcm/format.h>
#include <pcm/sample_format.h>

/** Wave file header format.
 *
 * https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
 * @note: 8-bit samples are stored as unsigned bytes,
 * 16-bit samples are stored as signed integers.
 * @note: The default byte ordering assumed for WAVE data files is
 * little-endian. Files written using the big-endian byte ordering scheme have
 * the identifier RIFX instead of RIFF.
 */
typedef struct wave_header {
	/** Should be 'R', 'I', 'F', 'F'. */
	char chunk_id[4];
#define CHUNK_ID "RIFF"

	/** Total size minus the first 8 bytes */
	uint32_t chunk_size;
	/** Should be 'W', 'A', 'V', 'E'. */
	char format[4];
#define FORMAT_STR "WAVE"

	/** Should be 'f', 'm', 't', ' '. */
	char subchunk1_id[4];
#define SUBCHUNK1_ID "fmt "

	/** Size of the ret of this subchunk. 16 for PCM file. */
	uint32_t subchunk1_size;
#define PCM_SUBCHUNK1_SIZE 16
	/** Format. 1 for Linear PCM */
	uint16_t audio_format;
#define FORMAT_LINEAR_PCM 1
	/** Number of channels. */
	uint16_t channels;
	/** Sampling rate. */
	uint32_t sampling_rate;
	/** Byte rate. */
	uint32_t byte_rate;
	/** Block align. Bytes in one block (samples for all channels). */
	uint16_t block_align;
	/** Bits per sample (one channel). */
	uint16_t sample_size;

	/** Should be 'd', 'a', 't', 'a'. */
	char subchunk2_id[4];
#define SUBCHUNK2_ID "data"
	/** Audio data size. */
	uint32_t subchunk2_size;
	/** Audio data. */
	uint8_t data[];

} wave_header_t;

errno_t wav_parse_header(const void *, const void**, size_t *, unsigned *,
    unsigned *, pcm_sample_format_t *, const char **);

void wav_init_header(wave_header_t *, pcm_format_t , size_t);
#endif
/**
 * @}
 */
