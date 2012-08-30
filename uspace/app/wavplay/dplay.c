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
#include <pcm/format.h>
#include <sys/mman.h>
#include <sys/time.h>

#include <stdio.h>
#include <macros.h>

#include "wave.h"
#include "dplay.h"

#define DEFAULT_DEVICE "/hw/pci0/00:01.0/sb16/pcm"
#define BUFFER_PARTS 2

typedef struct {
	struct {
		void *base;
		size_t size;
		void* position;
	} buffer;
	pcm_format_t f;
	FILE* source;
	volatile bool playing;
	fibril_mutex_t mutex;
	fibril_condvar_t cv;
	audio_pcm_sess_t *device;
} playback_t;

static void playback_initialize(playback_t *pb, audio_pcm_sess_t *sess)
{
	assert(sess);
	assert(pb);
	pb->buffer.base = NULL;
	pb->buffer.size = 0;
	pb->buffer.position = NULL;
	pb->playing = false;
	pb->source = NULL;
	pb->device = sess;
	fibril_mutex_initialize(&pb->mutex);
	fibril_condvar_initialize(&pb->cv);
}


static void device_event_callback(ipc_callid_t iid, ipc_call_t *icall, void* arg)
{
	async_answer_0(iid, EOK);
	playback_t *pb = arg;
	const size_t fragment_size = pb->buffer.size / BUFFER_PARTS;
	while (1) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		switch(IPC_GET_IMETHOD(call)) {
		case PCM_EVENT_PLAYBACK_STARTED:
		case PCM_EVENT_FRAMES_PLAYED:
			printf("%u frames: ", IPC_GET_ARG1(call));
			async_answer_0(callid, EOK);
			break;
		case PCM_EVENT_PLAYBACK_TERMINATED:
			printf("Playback terminated\n");
			fibril_mutex_lock(&pb->mutex);
			pb->playing = false;
			fibril_condvar_signal(&pb->cv);
			async_answer_0(callid, EOK);
			fibril_mutex_unlock(&pb->mutex);
			return;
		default:
			printf("Unknown event %d.\n", IPC_GET_IMETHOD(call));
			async_answer_0(callid, ENOTSUP);
			continue;

		}
		const size_t bytes = fread(pb->buffer.position, sizeof(uint8_t),
		   fragment_size, pb->source);
		printf("Copied from position %p size %zu/%zu\n",
		    pb->buffer.position, bytes, fragment_size);
		if (bytes == 0) {
			audio_pcm_last_playback_fragment(pb->device);
		}
		bzero(pb->buffer.position + bytes, fragment_size - bytes);
		pb->buffer.position += fragment_size;

		if (pb->buffer.position >= (pb->buffer.base + pb->buffer.size))
			pb->buffer.position -= pb->buffer.size;
	}
}

static void play_fragment(playback_t *pb)
{
	assert(pb);
	assert(pb->device);
	const size_t fragment_size = pb->buffer.size / BUFFER_PARTS;
	printf("Registering event callback\n");
	int ret = audio_pcm_register_event_callback(pb->device,
	    device_event_callback, pb);
	if (ret != EOK) {
		printf("Failed to register event callback.\n");
		return;
	}
	printf("Playing: %dHz, %s, %d channel(s).\n", pb->f.sampling_rate,
	    pcm_sample_format_str(pb->f.sample_format), pb->f.channels);
	const size_t bytes = fread(pb->buffer.base, sizeof(uint8_t),
	    fragment_size, pb->source);
	if (bytes != fragment_size)
		bzero(pb->buffer.base + bytes, fragment_size - bytes);
	printf("Initial: Copied from position %p size %zu/%zu\n",
	    pb->buffer.base, bytes, fragment_size);
	pb->buffer.position = pb->buffer.base + fragment_size;
	fibril_mutex_lock(&pb->mutex);
	const unsigned frames = pcm_format_size_to_frames(fragment_size, &pb->f);
	ret = audio_pcm_start_playback_fragment(pb->device, frames,
	    pb->f.channels, pb->f.sampling_rate, pb->f.sample_format);
	if (ret != EOK) {
		fibril_mutex_unlock(&pb->mutex);
		printf("Failed to start playback: %s.\n", str_error(ret));
		audio_pcm_unregister_event_callback(pb->device);
		return;
	}

	for (pb->playing = true; pb->playing;
	    fibril_condvar_wait(&pb->cv, &pb->mutex));

	fibril_mutex_unlock(&pb->mutex);
	printf("\n");
	audio_pcm_unregister_event_callback(pb->device);
}

static void play(playback_t *pb)
{
	assert(pb);
	assert(pb->device);
	pb->buffer.position = pb->buffer.base;
	printf("Playing: %dHz, %s, %d channel(s).\n", pb->f.sampling_rate,
	    pcm_sample_format_str(pb->f.sample_format), pb->f.channels);
	static suseconds_t work_time = 8000; /* 2 ms */
	size_t bytes = fread(pb->buffer.position, sizeof(uint8_t),
		    pb->buffer.size, pb->source);
	if (bytes == 0)
		return;
	audio_pcm_start_playback(pb->device,
	    pb->f.channels, pb->f.sampling_rate, pb->f.sample_format);
	do {
		size_t pos = 0;
		audio_pcm_get_buffer_pos(pb->device, &pos);
		useconds_t usecs = pcm_format_size_to_usec(bytes - pos, &pb->f);

		pb->buffer.position += bytes;

		printf("%u usecs to play %zu bytes from pos %zu.\n",
		    usecs, bytes, pos);
		async_usleep(usecs - work_time);
		audio_pcm_get_buffer_pos(pb->device, &pos);
		printf("Woke up at position %zu/%zu.\n", pos, pb->buffer.size);

		/* Remove any overflow */
		while (pb->buffer.position >= pb->buffer.base + pb->buffer.size)
			pb->buffer.position -= pb->buffer.size;

		if (bytes < pb->buffer.size) {
			const size_t remain = pb->buffer.size -
			    (pb->buffer.position - pb->buffer.base);
			/* This was the last part,
			 * zero 200 bytes or until the end of buffer. */
			bzero(pb->buffer.position, min(200, remain));
			if ((pb->buffer.base + pos) > pb->buffer.position) {
				printf("Overflow: %zu vs. %zu!\n",
				    pos, pb->buffer.position - pb->buffer.base);
			} else {
				udelay(pcm_format_size_to_usec(
				    pb->buffer.position - pb->buffer.base - pos,
				    &pb->f));
				audio_pcm_get_buffer_pos(pb->device, &pos);
			}
			printf("Stopped at %zu(%zu)/%zu\n",
			    pos, pb->buffer.position - pb->buffer.base,
			    pb->buffer.size);
			break;
		}
		/* copy first half */
		bytes = fread(pb->buffer.position, sizeof(uint8_t),
		    pb->buffer.size / 2, pb->source);
		if (bytes == 0)
			break;
		audio_pcm_get_buffer_pos(pb->device, &pos);
		printf("Half buffer copied at pos %zu", pos);
		/* Wait until the rest of the buffer is ready */
		udelay(pcm_format_size_to_usec(pb->buffer.size - pos, &pb->f));
		/* copy the other part of the buffer */
		audio_pcm_get_buffer_pos(pb->device, &pos);
		printf(" the other half copied at pos %zu\n", pos);
		bytes += fread(pb->buffer.position + bytes, sizeof(uint8_t),
			    pb->buffer.size / 2, pb->source);
	} while (1);
	audio_pcm_stop_playback(pb->device);
}

int dplay(const char *device, const char *file)
{
	int ret = EOK;
	if (str_cmp(device, "default") == 0)
		device = DEFAULT_DEVICE;
	audio_pcm_sess_t *session = audio_pcm_open(device);
	if (!session) {
		printf("Failed to connect to device %s.\n", device);
		return 1;
	}
	printf("Playing on device: %s.\n", device);
	if (audio_pcm_query_cap(session, AUDIO_CAP_PLAYBACK) <= 0) {
		printf("Device %s does not support playback\n", device);
		ret = ENOTSUP;
		goto close_session;
	}

	const char* info = NULL;
	ret = audio_pcm_get_info_str(session, &info);
	if (ret != EOK) {
		printf("Failed to get PCM info.\n");
		goto close_session;
	}
	printf("Playing on %s.\n", info);
	free(info);

	playback_t pb;
	playback_initialize(&pb, session);

	ret = audio_pcm_get_buffer(pb.device, &pb.buffer.base, &pb.buffer.size);
	if (ret != EOK) {
		printf("Failed to get PCM buffer: %s.\n", str_error(ret));
		goto close_session;
	}
	printf("Buffer: %p %zu.\n", pb.buffer.base, pb.buffer.size);
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
	const char *error;
	ret = wav_parse_header(&header, NULL, NULL,
	    &pb.f.channels, &pb.f.sampling_rate, &pb.f.sample_format, &error);
	if (ret != EOK) {
		printf("Error parsing wav header: %s.\n", error);
		fclose(pb.source);
		goto cleanup;
	}
	if (audio_pcm_query_cap(pb.device, AUDIO_CAP_BUFFER_POS) > 0) {
		play(&pb);
	} else {
		if (audio_pcm_query_cap(pb.device, AUDIO_CAP_INTERRUPT) > 0)
			play_fragment(&pb);
		else
			printf("Neither playing method is supported");
	}

cleanup:
	fclose(pb.source);
	munmap(pb.buffer.base, pb.buffer.size);
	audio_pcm_release_buffer(pb.device);
close_session:
	audio_pcm_close(session);
	return ret == EOK ? 0 : 1;
}
/**
 * @}
 */
