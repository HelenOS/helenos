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
#include <stdio.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <stdio.h>
#include <macros.h>

#include "wave.h"

#define DEFAULT_DEVICE "/hw/pci0/00:01.0/sb16/dsp"
#define SUBBUFFERS 4

typedef struct {
	struct {
		void *base;
		size_t size;
		unsigned id;
		void* position;
	} buffer;
	FILE* source;
} playback_t;


static void device_event_callback(ipc_callid_t id, ipc_call_t *call, void* arg)
{
	if (IPC_GET_IMETHOD(*call) != IPC_FIRST_USER_METHOD) {
		printf("Unknown event.\n");
		return;
	}
	playback_t *pb = arg;
	printf("Got device event!!!\n");
	const size_t buffer_part = pb->buffer.size / SUBBUFFERS;
	const size_t bytes = fread(pb->buffer.position, sizeof(uint8_t),
	   buffer_part, pb->source);
	if (bytes == 0)
		exit(0); // ugly but temporary
	pb->buffer.position += bytes;
	if (bytes != buffer_part)
		bzero(pb->buffer.position, buffer_part - bytes);
	pb->buffer.position += buffer_part - bytes;
	if (pb->buffer.position >= (pb->buffer.base + pb->buffer.size))
		pb->buffer.position = pb->buffer.base;
}


static void play(async_exch_t *device, playback_t *pb,
    unsigned sampling_rate, unsigned sample_size, unsigned channels, bool sign)
{
	assert(device);
	assert(pb);
	pb->buffer.position = pb->buffer.base;
	printf("Playing: %dHz, %d-bit samples, %d channel(s), %sSIGNED.\n",
	    sampling_rate, sample_size, channels, sign ? "": "UN");
	const size_t bytes = fread(pb->buffer.base, sizeof(uint8_t),
	    pb->buffer.size, pb->source);
	if (bytes != pb->buffer.size)
		return;
	printf("Buffer data ready.\n");
	sleep(10);
#if 0
	const size_t update_size = size / SUBBUFFERS;

	/* Time to play half the buffer. */
	const suseconds_t interval = 1000000 /
	    (sampling_rate /  (update_size / (channels * (sample_size / 8))));
	printf("Time to play half buffer: %ld us.\n", interval);
	/* Initialize buffer. */
	const size_t bytes = fread(buffer, sizeof(uint8_t), size, source);
	if (bytes != size)
		return;

	struct timeval time;
	gettimeofday(&time, NULL);
	printf("Starting playback.\n");
	int ret = audio_pcm_buffer_start_playback(device, buffer_id, SUBBUFFERS,
	    sampling_rate, sample_size, channels, sign);
	if (ret != EOK) {
		printf("Failed to start playback: %s.\n", str_error(ret));
		return;
	}
	void *buffer_place = buffer;
	while (true) {
		tv_add(&time, interval); /* Next update point */

		struct timeval current;
		gettimeofday(&current, NULL);

		const suseconds_t delay = min(tv_sub(&time, &current), interval);
		if (delay > 0)
			udelay(delay);

		const size_t bytes =
		    fread(buffer_place, sizeof(uint8_t), update_size, source);
		if (bytes == 0)
			break;
		if (bytes < update_size) {
			bzero(buffer_place + bytes, update_size - bytes);
		}
		buffer_place += update_size;

		if (buffer_place == buffer + size) {
			buffer_place = buffer;
		}
	}

	printf("Stopping playback.\n");
	ret = audio_pcm_buffer_stop_playback(device, buffer_id);
	if (ret != EOK) {
		printf("Failed to stop playback: %s.\n", str_error(ret));
	}
#endif
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
		printf("Failed to start session exchange.\n");
		async_hangup(session);
		return 1;
	}
	const char* info = NULL;
	ret = audio_pcm_buffer_get_info_str(exch, &info);
	if (ret != EOK) {
		printf("Failed to get PCM info.\n");
		async_exchange_end(exch);
		async_hangup(session);
		return 1;
	}
	printf("Playing on %s.\n", info);
	free(info);

	playback_t pb = {{0}, NULL};
	pb.buffer.size = 4096;
	printf("Requesting buffer: %p, %zu, %u.\n", pb.buffer.base, pb.buffer.size, pb.buffer.id);
	ret = audio_pcm_buffer_get_buffer(exch, &pb.buffer.base,
	    &pb.buffer.size, &pb.buffer.id, device_event_callback, &pb);
	if (ret != EOK) {
		printf("Failed to get PCM buffer: %s.\n", str_error(ret));
		async_exchange_end(exch);
		async_hangup(session);
		return 1;
	}
	printf("Buffer (%u): %p %zu.\n", pb.buffer.id, pb.buffer.base,
	    pb.buffer.size);

	pb.source = fopen(file, "rb");
	if (pb.source == NULL) {
		printf("Failed to open %s.\n", file);
		munmap(pb.buffer.base, pb.buffer.size);
		audio_pcm_buffer_release_buffer(exch, pb.buffer.id);
		async_exchange_end(exch);
		async_hangup(session);
		return 1;
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
		munmap(pb.buffer.base, pb.buffer.size);
		audio_pcm_buffer_release_buffer(exch, pb.buffer.id);
		async_exchange_end(exch);
		async_hangup(session);
		return 1;
	}

	play(exch, &pb, rate, sample_size, channels, sign);

	munmap(pb.buffer.base, pb.buffer.size);
	audio_pcm_buffer_release_buffer(exch, pb.buffer.id);
	async_exchange_end(exch);
	async_hangup(session);
	return 0;
}
/**
 * @}
 */
