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

/** @addtogroup dplay
 * @{
 */
/**
 * @file PCM playback audio devices
 */

#include <assert.h>
#include <errno.h>
#include <str_error.h>
#include <str.h>
#include <devman.h>
#include <audio_pcm_iface.h>
#include <fibril_synch.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <stdio.h>
#include <macros.h>

#include "wave.h"

#define DEFAULT_DEVICE "/hw/pci0/00:01.0/sb16/pcm"
#define SUBBUFFERS 2

const unsigned sampling_rate = 44100, sample_size = 16, channels = 2;
bool sign = true;

typedef struct {
	struct {
		void *base;
		size_t size;
		unsigned id;
		void* position;
	} buffer;
	FILE* file;
	async_exch_t *device;
} record_t;

static void record_initialize(record_t *rec, async_exch_t *exch)
{
	assert(exch);
	assert(rec);
	rec->buffer.id = 0;
	rec->buffer.base = NULL;
	rec->buffer.size = 0;
	rec->buffer.position = NULL;
	rec->file = NULL;
	rec->device = exch;
}


static void device_event_callback(ipc_callid_t iid, ipc_call_t *icall, void* arg)
{
	async_answer_0(iid, EOK);
	record_t *rec = arg;
	const size_t buffer_part = rec->buffer.size / SUBBUFFERS;
	while (1) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		if (IPC_GET_IMETHOD(call) != IPC_FIRST_USER_METHOD) {
			printf("Unknown event.\n");
			break;
		}
		const size_t bytes = fwrite(rec->buffer.position,
		   sizeof(uint8_t), buffer_part, rec->file);
		printf("%zu ", bytes);
		rec->buffer.position += buffer_part;

		if (rec->buffer.position >= (rec->buffer.base + rec->buffer.size))
			rec->buffer.position = rec->buffer.base;
		async_answer_0(callid, EOK);
	}
}


static void record(record_t *rec, unsigned sampling_rate, unsigned sample_size,
    unsigned channels, bool sign)
{
	assert(rec);
	assert(rec->device);
	rec->buffer.position = rec->buffer.base;
	printf("Recording: %dHz, %d-bit %ssigned samples, %d channel(s).\n",
	    sampling_rate, sample_size, sign ? "": "un", channels);
	int ret = audio_pcm_start_record(rec->device, rec->buffer.id,
	    SUBBUFFERS, sampling_rate, sample_size, channels, sign);
	if (ret != EOK) {
		printf("Failed to start recording: %s.\n", str_error(ret));
		return;
	}

	getchar();
	printf("\n");
	audio_pcm_stop_record(rec->device, rec->buffer.id);
}

int main(int argc, char *argv[])
{
	const char *device = DEFAULT_DEVICE;
	const char *file;
	switch (argc) {
	case 2:
		file = argv[1];
		break;
	case 3:
		device = argv[1];
		file = argv[2];
		break;
	default:
		printf("Usage: %s [device] file.\n", argv[0]);
		return 1;
	}

	devman_handle_t pcm_handle;
	int ret = devman_fun_get_handle(device, &pcm_handle, 0);
	if (ret != EOK) {
		printf("Failed to get device(%s) handle: %s.\n",
		    device, str_error(ret));
		return 1;
	}

	async_sess_t *session = devman_device_connect(
	    EXCHANGE_SERIALIZE, pcm_handle, IPC_FLAG_BLOCKING);
	if (!session) {
		printf("Failed to connect to device.\n");
		return 1;
	}

	async_exch_t *exch = async_exchange_begin(session);
	if (!exch) {
		ret = EPARTY;
		printf("Failed to start session exchange.\n");
		goto close_session;
	}
	const char* info = NULL;
	ret = audio_pcm_get_info_str(exch, &info);
	if (ret != EOK) {
		printf("Failed to get PCM info.\n");
		goto close_session;
	}
	printf("Playing on %s.\n", info);
	free(info);

	record_t rec;
	record_initialize(&rec, exch);

	ret = audio_pcm_get_buffer(rec.device, &rec.buffer.base,
	    &rec.buffer.size, &rec.buffer.id, device_event_callback, &rec);
	if (ret != EOK) {
		printf("Failed to get PCM buffer: %s.\n", str_error(ret));
		goto close_session;
	}
	printf("Buffer (%u): %p %zu.\n", rec.buffer.id, rec.buffer.base,
	    rec.buffer.size);
	uintptr_t ptr = 0;
	as_get_physical_mapping(rec.buffer.base, &ptr);
	printf("buffer mapped at %x.\n", ptr);

	rec.file = fopen(file, "wb");
	if (rec.file == NULL) {
		ret = ENOENT;
		printf("Failed to open %s.\n", file);
		goto cleanup;
	}
	wave_header_t header = {
		.chunk_id = CHUNK_ID,
		.format = FORMAT_STR,
		.subchunk1_id = SUBCHUNK1_ID,
		.subchunk1_size = PCM_SUBCHUNK1_SIZE,
		.audio_format = FORMAT_LINEAR_PCM,
		.channels = channels,
		.sampling_rate = sampling_rate,
		.sample_size = sample_size,
		.byte_rate = (sampling_rate / 8) * channels,
		.block_align = (sampling_rate / 8) * channels,
		.subchunk2_id = SUBCHUNK2_ID,
	};
	fwrite(&header, sizeof(header), 1, rec.file);
	record(&rec, sampling_rate, sample_size, channels, sign);
	fclose(rec.file);

cleanup:
	munmap(rec.buffer.base, rec.buffer.size);
	audio_pcm_release_buffer(exch, rec.buffer.id);
close_session:
	async_exchange_end(exch);
	async_hangup(session);
	return ret == EOK ? 0 : 1;
}
/**
 * @}
 */
