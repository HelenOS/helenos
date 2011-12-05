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

int wav_parse_header(void *file, void ** data, size_t *data_size,
    unsigned *sampling_rate, unsigned *sample_size, unsigned *channels,
    bool *sign, const char **error)
{
	if (!file || !data || !data_size) {
		if (error)
			*error = "file, data and data_size must be specified";
		return EINVAL;
	}

	wave_header_t *header = file;
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

	if (uint16_t_le2host(header->subchunk1_size) != PCM_SUBCHUNK1_SIZE) {
		if (error)
			*error = "invalid subchunk1 size";
		return EINVAL;
	}

	if (uint16_t_le2host(header->audio_format) != FORMAT_LINEAR_PCM) {
		if (error)
			*error = "unknown format";
		return ENOTSUP;
	}

	if (str_lcmp(header->subchunk2_id, SUBCHUNK2_ID, 4) != 0) {
		if (error)
			*error = "invalid subchunk2 id";
		return EINVAL;
	}

	*data = header->data;
	*data_size = uint32_t_le2host(header->subchunk2_size);

	if (sampling_rate)
		*sampling_rate = uint32_t_le2host(header->sampling_rate);
	if (sample_size)
		*sample_size = uint32_t_le2host(header->sample_size);
	if (channels)
		*channels = uint16_t_le2host(header->channels);
	if (sign)
		*sign = uint32_t_le2host(header->sample_size) == 16
		    ? true : false;
	if (error)
		*error = "no error";

	return EOK;
}
/**
 * @}
 */
