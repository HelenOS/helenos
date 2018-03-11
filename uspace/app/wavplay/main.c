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

#include <assert.h>
#include <errno.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include <str_error.h>
#include <stdio.h>
#include <hound/client.h>
#include <pcm/sample_format.h>
#include <getopt.h>

#include "dplay.h"
#include "drec.h"
#include "wave.h"

#define READ_SIZE   (32 * 1024)
#define STREAM_BUFFER_SIZE   (64 * 1024)

/**
 * Play audio file using a new stream on provided context.
 * @param ctx Provided context.
 * @param filename File to play.
 * @return Error code.
 */
static errno_t hplay_ctx(hound_context_t *ctx, const char *filename)
{
	printf("Hound context playback: %s\n", filename);
	FILE *source = fopen(filename, "rb");
	if (!source) {
		printf("Failed to open file %s\n", filename);
		return EINVAL;
	}

	/* Read and parse WAV header */
	wave_header_t header;
	size_t read = fread(&header, sizeof(header), 1, source);
	if (read != 1) {
		printf("Failed to read WAV header: %zu\n", read);
		fclose(source);
		return EIO;
	}
	pcm_format_t format;
	const char *error;
	errno_t ret = wav_parse_header(&header, NULL, NULL, &format.channels,
	    &format.sampling_rate, &format.sample_format, &error);
	if (ret != EOK) {
		printf("Error parsing `%s' wav header: %s.\n", filename, error);
		fclose(source);
		return EINVAL;
	}

	printf("File `%s' format: %u channel(s), %uHz, %s.\n", filename,
	    format.channels, format.sampling_rate,
	    pcm_sample_format_str(format.sample_format));

	/* Allocate buffer and create new context */
	char * buffer = malloc(READ_SIZE);
	if (!buffer) {
		fclose(source);
		return ENOMEM;
	}
	hound_stream_t *stream = hound_stream_create(ctx,
	    HOUND_STREAM_DRAIN_ON_EXIT, format, STREAM_BUFFER_SIZE);

	/* Read and play */
	while ((read = fread(buffer, sizeof(char), READ_SIZE, source)) > 0) {
		ret = hound_stream_write(stream, buffer, read);
		if (ret != EOK) {
			printf("Failed to write to hound stream: %s\n",
			    str_error(ret));
			break;
		}
	}

	/* Cleanup */
	free(buffer);
	fclose(source);
	return ret;
}

/**
 * Play audio file via hound server.
 * @param filename File to play.
 * @return Error code
 */
static errno_t hplay(const char *filename)
{
	printf("Hound playback: %s\n", filename);
	FILE *source = fopen(filename, "rb");
	if (!source) {
		printf("Failed to open file %s\n", filename);
		return EINVAL;
	}

	/* Read and parse WAV header */
	wave_header_t header;
	size_t read = fread(&header, sizeof(header), 1, source);
	if (read != 1) {
		printf("Failed to read WAV header: %zu\n", read);
		fclose(source);
		return EIO;
	}
	pcm_format_t format;
	const char *error;
	errno_t ret = wav_parse_header(&header, NULL, NULL, &format.channels,
	    &format.sampling_rate, &format.sample_format, &error);
	if (ret != EOK) {
		printf("Error parsing `%s' wav header: %s.\n", filename, error);
		fclose(source);
		return EINVAL;
	}
	printf("File `%s' format: %u channel(s), %uHz, %s.\n", filename,
	    format.channels, format.sampling_rate,
	    pcm_sample_format_str(format.sample_format));

	/* Connect new playback context */
	hound_context_t *hound = hound_context_create_playback(filename,
	    format, STREAM_BUFFER_SIZE);
	if (!hound) {
		printf("Failed to create HOUND context\n");
		fclose(source);
		return ENOMEM;
	}

	ret = hound_context_connect_target(hound, HOUND_DEFAULT_TARGET);
	if (ret != EOK) {
		printf("Failed to connect to default target: %s\n",
		    str_error(ret));
		hound_context_destroy(hound);
		fclose(source);
		return ret;
	}

	/* Read and play */
	static char buffer[READ_SIZE];
	while ((read = fread(buffer, sizeof(char), READ_SIZE, source)) > 0) {
		ret = hound_write_main_stream(hound, buffer, read);
		if (ret != EOK) {
			printf("Failed to write to main context stream: %s\n",
			    str_error(ret));
			break;
		}
	}

	/* Cleanup */
	hound_context_destroy(hound);
	fclose(source);
	return ret;
}

/**
 * Helper structure for playback in separate fibrils
 */
typedef struct {
	hound_context_t *ctx;
	atomic_t *count;
	const char *file;
} fib_play_t;

/**
 * Fibril playback wrapper.
 * @param arg Argument, pointer to playback helper structure.
 * @return Error code.
 */
static errno_t play_wrapper(void *arg)
{
	assert(arg);
	fib_play_t *p = arg;
	const errno_t ret = hplay_ctx(p->ctx, p->file);
	atomic_dec(p->count);
	free(arg);
	return ret;
}

/**
 * Array of supported commandline options
 */
static const struct option opts[] = {
	{"device", required_argument, 0, 'd'},
	{"parallel", no_argument, 0, 'p'},
	{"record", no_argument, 0, 'r'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};

/**
 * Print usage help.
 * @param name Name of the program.
 */
static void print_help(const char* name)
{
	printf("Usage: %s [options] file [files...]\n", name);
	printf("supported options:\n");
	printf("\t -h, --help\t Print this help.\n");
	printf("\t -r, --record\t Start recording instead of playback. "
	    "(Not implemented)\n");
	printf("\t -d, --device\t Use specified device instead of the sound "
	    "service. Use location path or a special device `default'\n");
	printf("\t -p, --parallel\t Play given files in parallel instead of "
	    "sequentially (does not work with -d).\n");
}

int main(int argc, char *argv[])
{
	const char *device = "default";
	int idx = 0;
	bool direct = false, record = false, parallel = false;
	optind = 0;
	int ret = 0;

	/* Parse command line options */
	while (ret != -1) {
		ret = getopt_long(argc, argv, "d:prh", opts, &idx);
		switch (ret) {
		case 'd':
			direct = true;
			device = optarg;
			break;
		case 'r':
			record = true;
			break;
		case 'p':
			parallel = true;
			break;
		case 'h':
			print_help(*argv);
			return 0;
		}
	}

	if (parallel && direct) {
		printf("Parallel playback is available only if using sound "
		    "server (no -d)\n");
		print_help(*argv);
		return 1;
	}

	if (optind == argc) {
		printf("Not enough arguments.\n");
		print_help(*argv);
		return 1;
	}

	/* Init parallel playback variables */
	hound_context_t *hound_ctx = NULL;
	atomic_t playcount;
	atomic_set(&playcount, 0);

	/* Init parallel playback context if necessary */
	if (parallel) {
		hound_ctx = hound_context_create_playback("wavplay",
		    AUDIO_FORMAT_DEFAULT, STREAM_BUFFER_SIZE);
		if (!hound_ctx) {
			printf("Failed to create global hound context\n");
			return 1;
		}
		const errno_t ret = hound_context_connect_target(hound_ctx,
		    HOUND_DEFAULT_TARGET);
		if (ret != EOK) {
			printf("Failed to connect hound context to default "
			   "target.\n");
			hound_context_destroy(hound_ctx);
			return 1;
		}
	}

	/* play or record all files */
	for (int i = optind; i < argc; ++i) {
		const char *file = argv[i];

		printf("%s (%d/%d) %s\n", record ? "Recording" : "Playing",
		    i - optind + 1, argc - optind, file);
		if (record) {
			if (direct) {
				drecord(device, file);
				continue;
			} else {
				printf("Indirect recording is not supported "
				    "yet.\n");
				break;
			}
		}

		if (direct) {
			dplay(device, file);
		} else {
			if (parallel) {
				/* Start new fibril for parallel playback */
				fib_play_t *data = malloc(sizeof(fib_play_t));
				if (!data) {
					printf("Playback of %s failed.\n",
						file);
					continue;
				}
				data->file = file;
				data->count = &playcount;
				data->ctx = hound_ctx;
				fid_t fid = fibril_create(play_wrapper, data);
				atomic_inc(&playcount);
				fibril_add_ready(fid);
			} else {
				hplay(file);
			}
		}
	}

	/* Wait for all fibrils to finish */
	while (atomic_get(&playcount) > 0)
		async_usleep(1000000);

	/* Destroy parallel playback context, if initialized */
	if (hound_ctx)
		hound_context_destroy(hound_ctx);
	return 0;
}
/**
 * @}
 */
