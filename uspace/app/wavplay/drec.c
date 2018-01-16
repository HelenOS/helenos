/*
 * Copyright (c) 2013 Jan Vesely
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

#include <assert.h>
#include <errno.h>
#include <str_error.h>
#include <audio_pcm_iface.h>
#include <pcm/format.h>
#include <stdio.h>
#include <as.h>
#include <inttypes.h>
#include <str.h>

#include "wave.h"
#include "drec.h"


#define BUFFER_PARTS   16

/** Recording format */
static const pcm_format_t format = {
	.sampling_rate = 44100,
	.channels = 2,
	.sample_format = PCM_SAMPLE_SINT16_LE,
};

/** Recording helper structure */
typedef struct {
	struct {
		void *base;
		size_t size;
		unsigned id;
		void* position;
	} buffer;
	FILE* file;
	audio_pcm_sess_t *device;
} record_t;

/**
 * Initialize recording helper structure.
 * @param rec Recording structure.
 * @param sess Session to IPC device.
 */
static void record_initialize(record_t *rec, audio_pcm_sess_t *sess)
{
	assert(sess);
	assert(rec);
	rec->buffer.base = NULL;
	rec->buffer.size = 0;
	rec->buffer.position = NULL;
	rec->file = NULL;
	rec->device = sess;
}

/**
 * Recording callback. Writes recorded data.
 * @param iid IPC call id.
 * @param icall Poitner to IPC call structure.
 * @param arg Argument. Poitner to recording helper structure.
 */
static void device_event_callback(ipc_callid_t iid, ipc_call_t *icall, void* arg)
{
	async_answer_0(iid, EOK);
	record_t *rec = arg;
	const size_t buffer_part = rec->buffer.size / BUFFER_PARTS;
	bool record = true;
	while (record) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		switch(IPC_GET_IMETHOD(call)) {
		case PCM_EVENT_CAPTURE_TERMINATED:
			printf("Recording terminated\n");
			record = false;
			break;
		case PCM_EVENT_FRAMES_CAPTURED:
			printf("%" PRIun " frames\n", IPC_GET_ARG1(call));
			break;
		default:
			printf("Unknown event %" PRIun ".\n", IPC_GET_IMETHOD(call));
			async_answer_0(callid, ENOTSUP);
			continue;
		}

		if (!record) {
			async_answer_0(callid, EOK);
			break;
		}

		/* Write directly from device buffer to file */
		const size_t bytes = fwrite(rec->buffer.position,
		   sizeof(uint8_t), buffer_part, rec->file);
		printf("%zu ", bytes);
		rec->buffer.position += buffer_part;

		if (rec->buffer.position >= (rec->buffer.base + rec->buffer.size))
			rec->buffer.position = rec->buffer.base;
		async_answer_0(callid, EOK);
	}
}

/**
 * Start fragment based recording.
 * @param rec Recording helper structure.
 * @param f PCM format
 */
static void record_fragment(record_t *rec, pcm_format_t f)
{
	assert(rec);
	assert(rec->device);
	errno_t ret = audio_pcm_register_event_callback(rec->device,
	    device_event_callback, rec);
	if (ret != EOK) {
		printf("Failed to register for events: %s.\n", str_error(ret));
		return;
	}
	rec->buffer.position = rec->buffer.base;
	printf("Recording: %dHz, %s, %d channel(s).\n", f.sampling_rate,
	    pcm_sample_format_str(f.sample_format), f.channels);
	const unsigned frames =
		pcm_format_size_to_frames(rec->buffer.size / BUFFER_PARTS, &f);
	ret = audio_pcm_start_capture_fragment(rec->device,
	    frames, f.channels, f.sampling_rate, f.sample_format);
	if (ret != EOK) {
		printf("Failed to start recording: %s.\n", str_error(ret));
		return;
	}

	getchar();
	printf("\n");
	audio_pcm_stop_capture(rec->device);
	/* XXX Control returns even before we can be sure callbacks finished */
	printf("Delay before playback termination\n");
	async_usleep(1000000);
	printf("Terminate playback\n");
}

/**
 * Record directly from a device to a file.
 * @param device The device.
 * @param file The file.
 * @return 0 on succes, non-zero on failure.
 */
int drecord(const char *device, const char *file)
{
	errno_t ret = EOK;
	audio_pcm_sess_t *session = NULL;
	sysarg_t val;
	if (str_cmp(device, "default") == 0) {
		session = audio_pcm_open_default();
	} else {
		session = audio_pcm_open(device);
	}
	if (!session) {
		printf("Failed to connect to device %s.\n", device);
		return 1;
	}
	printf("Recording on device: %s.\n", device);
	ret = audio_pcm_query_cap(session, AUDIO_CAP_CAPTURE, &val);
	if (ret != EOK || !val) {
		printf("Device %s does not support recording\n", device);
		ret = ENOTSUP;
		goto close_session;
	}

	char* info = NULL;
	ret = audio_pcm_get_info_str(session, &info);
	if (ret != EOK) {
		printf("Failed to get PCM info.\n");
		goto close_session;
	}
	printf("Capturing on %s.\n", info);
	free(info);

	record_t rec;
	record_initialize(&rec, session);

	ret = audio_pcm_get_buffer(rec.device, &rec.buffer.base,
	    &rec.buffer.size);
	if (ret != EOK) {
		printf("Failed to get PCM buffer: %s.\n", str_error(ret));
		goto close_session;
	}
	printf("Buffer: %p %zu.\n", rec.buffer.base, rec.buffer.size);

	rec.file = fopen(file, "w");
	if (rec.file == NULL) {
		ret = ENOENT;
		printf("Failed to open file: %s.\n", file);
		goto cleanup;
	}

	wave_header_t header;
	fseek(rec.file, sizeof(header), SEEK_SET);
	if (ret != EOK) {
		printf("Error parsing wav header\n");
		goto cleanup;
	}
	ret = audio_pcm_query_cap(rec.device, AUDIO_CAP_INTERRUPT, &val);
	if (ret == EOK && val)
		record_fragment(&rec, format);
	else
		printf("Recording method is not supported");
	//TODO consider buffer position interface

	wav_init_header(&header, format, ftell(rec.file) - sizeof(header));
	fseek(rec.file, 0, SEEK_SET);
	fwrite(&header, sizeof(header), 1, rec.file);

cleanup:
	fclose(rec.file);
	as_area_destroy(rec.buffer.base);
	audio_pcm_release_buffer(rec.device);
close_session:
	audio_pcm_close(session);
	return ret == EOK ? 0 : 1;
}
/**
 * @}
 */
