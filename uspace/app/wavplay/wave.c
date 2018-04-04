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
/**
 * @file PCM playback audio devices
 */

#include <byteorder.h>
#include <str.h>
#include <errno.h>

#include "wave.h"

/**
 * Parse wav header data.
 * @param[in] hdata Header data to parse.
 * @param[out] data Pointer to audio data.
 * @param[out] data_size Size of the data after the header.
 * @param[out] channels Number of channels in stored audio format.
 * @param[out] sampling_rate Sampling rate of the store data.
 * @param[out] format Sample format.
 * @param[out] error String representatoin of error, if any.
 * @return Error code.
 *
 * Does sanity checks and endian conversion.
 */
errno_t wav_parse_header(const void *hdata, const void **data, size_t *data_size,
    unsigned *channels, unsigned *sampling_rate, pcm_sample_format_t *format,
    const char **error)
{
	if (!hdata) {
		if (error)
			*error = "no header";
		return EINVAL;
	}

	const wave_header_t *header = hdata;
	if (str_lcmp(header->chunk_id, CHUNK_ID, 4) != 0) {
		if (error)
			*error = "invalid chunk id";
		return EINVAL;
	}

	if (str_lcmp(header->format, FORMAT_STR, 4) != 0) {
		if (error)
			*error = "invalid format string";
		return EINVAL;
	}

	if (str_lcmp(header->subchunk1_id, SUBCHUNK1_ID, 4) != 0) {
		if (error)
			*error = "invalid subchunk1 id";
		return EINVAL;
	}

	if (uint32_t_le2host(header->subchunk1_size) != PCM_SUBCHUNK1_SIZE) {
		//TODO subchunk 1 sizes other than 16 are allowed ( 18, 40)
		//http://www-mmsp.ece.mcgill.ca/documents/AudioFormats/WAVE/WAVE.html
		if (error)
			*error = "invalid subchunk1 size";
//		return EINVAL;
	}

	if (uint16_t_le2host(header->audio_format) != FORMAT_LINEAR_PCM) {
		if (error)
			*error = "unknown format";
		return ENOTSUP;
	}

	if (str_lcmp(header->subchunk2_id, SUBCHUNK2_ID, 4) != 0) {
		//TODO basedd on subchunk1 size, we might be reading wrong
		//offset
		if (error)
			*error = "invalid subchunk2 id";
//		return EINVAL;
	}


	//TODO data and data_size are incorrect in extended wav formats
	//pcm params are OK
	if (data)
		*data = header->data;
	if (data_size)
		*data_size = uint32_t_le2host(header->subchunk2_size);

	if (sampling_rate)
		*sampling_rate = uint32_t_le2host(header->sampling_rate);
	if (channels)
		*channels = uint16_t_le2host(header->channels);
	if (format) {
		const unsigned size = uint16_t_le2host(header->sample_size);
		switch (size) {
		case 8:
			*format = PCM_SAMPLE_UINT8;
			break;
		case 16:
			*format = PCM_SAMPLE_SINT16_LE;
			break;
		case 24:
			*format = PCM_SAMPLE_SINT24_LE;
			break;
		case 32:
			*format = PCM_SAMPLE_SINT32_LE;
			break;
		default:
			*error = "Unknown format";
			return ENOTSUP;
		}
	}
	if (error)
		*error = "no error";

	return EOK;
}

/**
 * Initialize wave fromat ehader structure.
 * @param header Structure to initialize.
 * @param format Desired PCM format
 * @param size Size of the stored data.
 *
 * Initializes format specific elements and covnerts endian
 */
void wav_init_header(wave_header_t *header, pcm_format_t format, size_t size)
{
	assert(header);
#define COPY_STR(dst, src)   memcpy(dst, src, str_size(src))

	COPY_STR(&header->chunk_id, CHUNK_ID);
	COPY_STR(&header->format, FORMAT_STR);
	COPY_STR(&header->subchunk1_id, SUBCHUNK1_ID);
	header->subchunk1_size = host2uint16_t_le(PCM_SUBCHUNK1_SIZE);
	header->audio_format = host2uint16_t_le(FORMAT_LINEAR_PCM);

	COPY_STR(&header->subchunk2_id, SUBCHUNK2_ID);
	header->subchunk2_size = host2uint32_t_le(size);
	header->sampling_rate = host2uint32_t_le(format.sampling_rate);
	header->channels = host2uint32_t_le(format.channels);
	header->sample_size =
	    host2uint16_t_le(pcm_sample_format_size(format.sample_format) * 8);
}
/**
 * @}
 */
