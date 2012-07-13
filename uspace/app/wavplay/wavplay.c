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

/** @addtogroup wavplay
 * @{
 */
/**
 * @file PCM playback audio devices
 */

#include <async.h>
#include <assert.h>
#include <errno.h>
#include <str_error.h>
#include <str.h>
#include <devman.h>
#include <fibril_synch.h>
#include <stdio.h>
#include <sys/mman.h>
#include <loc.h>

#include <pcm_sample_format.h>

#include <stdio.h>
#include <macros.h>

#include "wave.h"

#define SERVICE "audio/hound"
#define DEFAULT_DEVICE "default" //"devices/\\hw\\pci0\\00:01.0\\sb16\\pcm"
#define SUBBUFFERS 2
#define NAME_MAX 32
char name[NAME_MAX + 1];

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
	async_exch_t *server;
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
	pb->server = exch;
	fibril_mutex_initialize(&pb->mutex);
	fibril_condvar_initialize(&pb->cv);
}


static void data_callback(ipc_callid_t iid, ipc_call_t *icall, void* arg)
{
	async_answer_0(iid, EOK);
	playback_t *pb = arg;

	while (1) {
		size_t size = 0;
		if (!async_data_read_receive(&iid, &size)) {
			printf("Data request failed");
			continue;
		}
//		printf("Server asked for more(%zu) data\n", size);
		if (pb->buffer.size < size) {
			printf("Reallocating buffer: %zu -> %zu\n",
			    pb->buffer.size, size);
			pb->buffer.base = realloc(pb->buffer.base, size);
			pb->buffer.size = size;
		}
		const size_t bytes = fread(pb->buffer.base, sizeof(uint8_t),
		   size, pb->source);
//		printf("%zu bytes ready\n", bytes);
		if (bytes < pb->buffer.size) {
			bzero(pb->buffer.base + bytes, size - bytes);
		}
		async_data_read_finalize(iid, pb->buffer.base, size);
		if (bytes == 0) {
			/* Disconnect */
			aid_t id = async_send_0(pb->server,
			    IPC_FIRST_USER_METHOD + 5, NULL);
			async_data_write_start(pb->server, name, str_size(name) + 1);
			async_data_write_start(pb->server, DEFAULT_DEVICE,
			    1+str_size(DEFAULT_DEVICE));
			async_wait_for(id, NULL);

			printf("\nPlayback terminated\n");
			fibril_mutex_lock(&pb->mutex);
			pb->playing = false;
			fibril_condvar_signal(&pb->cv);
			fibril_mutex_unlock(&pb->mutex);
			return;
		}

	}
}

static void play(playback_t *pb, unsigned channels, unsigned rate, pcm_sample_format_t format)
{
	assert(pb);
	/* Create playback client */
	aid_t id = async_send_3(pb->server, IPC_FIRST_USER_METHOD, channels,
	    rate, format, NULL);
	int ret = async_data_write_start(pb->server, name, str_size(name) + 1);
	if (ret != EOK) {
		printf("Failed to send client name: %s\n", str_error(ret));
		async_forget(id);
		return;
	}
	ret = async_connect_to_me(pb->server, 0, 0, 0, data_callback, pb);
	if (ret != EOK) {
		printf("Failed to establish callback: %s\n", str_error(ret));
		async_forget(id);
		return;
	}
	async_wait_for(id, (sysarg_t*)&ret);
	if (ret != EOK) {
		printf("Failed to create client: %s\n", str_error(ret));
		async_forget(id);
		return;
	}

	/* Connect */
	id = async_send_0(pb->server, IPC_FIRST_USER_METHOD + 4, NULL);
	async_data_write_start(pb->server, name, str_size(name) + 1);
	async_data_write_start(pb->server, DEFAULT_DEVICE, 1+str_size(DEFAULT_DEVICE));
	async_wait_for(id, NULL);

	fibril_mutex_lock(&pb->mutex);

	for (pb->playing = true; pb->playing;
	    fibril_condvar_wait(&pb->cv, &pb->mutex));
	fibril_mutex_unlock(&pb->mutex);
}

int main(int argc, char *argv[])
{
	if (argc != 2)
		return 1;
	const char *file = argv[1];

	service_id_t id = 0;
	int ret = loc_service_get_id(SERVICE, &id, 0);
	if (ret != EOK) {
		printf("Failed to get hound service id\n");
		return 1;
	}

	task_id_t tid = task_get_id();
	snprintf(name, NAME_MAX, "%s%" PRIu64 ":%s", argv[0], tid, file);

	printf("Client name: %s\n", name);

	async_sess_t *sess = loc_service_connect(EXCHANGE_SERIALIZE, id, 0);
	if (!sess) {
		printf("Failed to connect to hound service\n");
		return 1;
	}

	async_exch_t *exch = async_exchange_begin(sess);
	if (!exch) {
		printf("Failed to create exchange\n");
		async_hangup(sess);
		return 1;
	}


	playback_t pb;
	playback_initialize(&pb, exch);
	pb.source = fopen(file, "rb");
	if (pb.source == NULL) {
		ret = ENOENT;
		printf("Failed to open %s.\n", file);
		goto cleanup;
	}
	wave_header_t header;
	fread(&header, sizeof(header), 1, pb.source);
	unsigned rate, channels;
	pcm_sample_format_t format;
	const char *error;
	ret = wav_parse_header(&header, NULL, NULL, &channels, &rate, &format,
	    &error);
	if (ret != EOK) {
		printf("Error parsing wav header: %s.\n", error);
		fclose(pb.source);
		goto cleanup;
	}

	play(&pb, channels, rate, format);

cleanup:
	async_exchange_end(exch);

	async_hangup(sess);

	return 0;
}
/**
 * @}
 */
