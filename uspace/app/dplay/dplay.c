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

#include <assert.h>
#include <errno.h>
#include <str_error.h>
#include <str.h>
#include <devman.h>
#include <audio_pcm_buffer_iface.h>
#include <fibril_synch.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <stdio.h>
#include <macros.h>

#include "wave.h"

#define DEFAULT_DEVICE "/hw/pci0/00:01.0/sb16/dsp"
#define SUBBUFFERS 2

typedef struct {
	struct {
		void *base;
		size_t size;
		unsigned id;
		void* position;
	} buffer;
	FILE* source;
	volatile bool playing;
	fibril_mutex_t mutex;
	fibril_condvar_t cv;
	async_exch_t *device;
} playback_t;

static void playback_initialize(playback_t *pb, async_exch_t *exch)
{
	assert(exch);
	assert(pb);
	pb->buffer.id = 0;
	pb->buffer.base = NULL;
	pb->buffer.size = 0;
	pb->buffer.position = NULL;
	pb->playing = false;
	pb->source = NULL;
	pb->device = exch;
	fibril_mutex_initialize(&pb->mutex);
	fibril_condvar_initialize(&pb->cv);
}


static void device_event_callback(ipc_callid_t iid, ipc_call_t *icall, void* arg)
{
	async_answer_0(iid, EOK);
	playback_t *pb = arg;
	const size_t buffer_part = pb->buffer.size / SUBBUFFERS;
	while (1) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		if (IPC_GET_IMETHOD(call) != IPC_FIRST_USER_METHOD) {
			printf("Unknown event.\n");
			break;
		}
		printf("+");
		const size_t bytes = fread(pb->buffer.position, sizeof(uint8_t),
		   buffer_part, pb->source);
		bzero(pb->buffer.position + bytes, buffer_part - bytes);
		pb->buffer.position += buffer_part;

		if (pb->buffer.position >= (pb->buffer.base + pb->buffer.size))
			pb->buffer.position = pb->buffer.base;
		async_answer_0(callid, EOK);
		if (bytes == 0) {
			pb->playing = false;
			fibril_condvar_signal(&pb->cv);
		}
	}
}


static void play(playback_t *pb, unsigned sampling_rate, unsigned sample_size,
    unsigned channels, bool sign)
{
	assert(pb);
	assert(pb->device);
	pb->buffer.position = pb->buffer.base;
	printf("Playing: %dHz, %d-bit %ssigned samples, %d channel(s).\n",
	    sampling_rate, sample_size, sign ? "": "un", channels);
	const size_t bytes = fread(pb->buffer.base, sizeof(uint8_t),
	    pb->buffer.size, pb->source);
	if (bytes != pb->buffer.size)
		bzero(pb->buffer.base + bytes, pb->buffer.size - bytes);
	printf("Buffer data ready.\n");
	fibril_mutex_lock(&pb->mutex);
	int ret = audio_pcm_buffer_start_playback(pb->device, pb->buffer.id,
	    SUBBUFFERS, sampling_rate, sample_size, channels, sign);
	if (ret != EOK) {
		fibril_mutex_unlock(&pb->mutex);
		printf("Failed to start playback: %s.\n", str_error(ret));
		return;
	}

	for (pb->playing = true; pb->playing;
	    fibril_condvar_wait(&pb->cv, &pb->mutex));

	audio_pcm_buffer_stop_playback(pb->device, pb->buffer.id);
	fibril_condvar_wait(&pb->cv, &pb->mutex);
	printf("\n");
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
	ret = audio_pcm_buffer_get_info_str(exch, &info);
	if (ret != EOK) {
		printf("Failed to get PCM info.\n");
		goto close_session;
	}
	printf("Playing on %s.\n", info);
	free(info);

	playback_t pb;
	playback_initialize(&pb, exch);

	ret = audio_pcm_buffer_get_buffer(pb.device, &pb.buffer.base,
	    &pb.buffer.size, &pb.buffer.id, device_event_callback, &pb);
	if (ret != EOK) {
		printf("Failed to get PCM buffer: %s.\n", str_error(ret));
		goto close_session;
	}
	printf("Buffer (%u): %p %zu.\n", pb.buffer.id, pb.buffer.base,
	    pb.buffer.size);
	uintptr_t ptr = 0;
	as_get_physical_mapping(pb.buffer.base, &ptr);
	printf("buffer mapped at %x.\n", ptr);

	pb.source = fopen(file, "rb");
	if (pb.source == NULL) {
		ret = ENOENT;
		printf("Failed to open %s.\n", file);
		goto cleanup;
	}
	wave_header_t header;
	fread(&header, sizeof(header), 1, pb.source);
	unsigned rate, sample_size, channels;
	bool sign;
	const char *error;
	ret = wav_parse_header(&header, NULL, NULL, &rate, &sample_size,
	    &channels, &sign, &error);
	if (ret != EOK) {
		printf("Error parsing wav header: %s.\n", error);
		fclose(pb.source);
		goto cleanup;
	}

	play(&pb, rate, sample_size, channels, sign);

cleanup:
	munmap(pb.buffer.base, pb.buffer.size);
	audio_pcm_buffer_release_buffer(exch, pb.buffer.id);
close_session:
	async_exchange_end(exch);
	async_hangup(session);
	return ret == EOK ? 0 : 1;
}
/**
 * @}
 */
