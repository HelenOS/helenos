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
#include <str_error.h>
#include <stdio.h>
#include <hound/client.h>
#include <pcm/sample_format.h>
#include <getopt.h>

#include "dplay.h"
#include "wave.h"

#define BUFFER_SIZE (32 * 1024)


static int hplay(const char *filename)
{
	printf("Hound playback: %s\n", filename);
	FILE *source = fopen(filename, "rb");
	if (!source) {
		printf("Failed to open file %s\n", filename);
		return EINVAL;
	}
	wave_header_t header;
	size_t read = fread(&header, sizeof(header), 1, source);
	if (read != 1) {
		printf("Failed to read WAV header: %zu\n", read);
		fclose(source);
		return EIO;
	}
	pcm_format_t format;
	const char *error;
	int ret = wav_parse_header(&header, NULL, NULL, &format.channels,
	    &format.sampling_rate, &format.sample_format, &error);
	if (ret != EOK) {
		printf("Error parsing wav header: %s.\n", error);
		fclose(source);
		return EINVAL;
	}
	hound_context_t *hound = hound_context_create_playback(filename,
	    format, BUFFER_SIZE * 2);
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
	static char buffer[BUFFER_SIZE];
	while ((read = fread(buffer, sizeof(char), BUFFER_SIZE, source)) > 0) {
		ret = hound_write_main_stream(hound, buffer, read);
		if (ret != EOK) {
			printf("Failed to write to main context stream: %s\n",
			    str_error(ret));
			break;
		}
	}
	hound_context_destroy(hound);
	fclose(source);
	return ret;
}

static const struct option opts[] = {
	{"device", required_argument, 0, 'd'},
	{"record", no_argument, 0, 'r'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
};

static void print_help(const char* name)
{
	printf("Usage: %s [options] file [files...]\n", name);
	printf("supported options:\n");
	printf("\t -h, --help\t Print this help.\n");
	printf("\t -r, --record\t Start recording instead of playback. "
	    "(Not implemented)\n");
	printf("\t -d, --device\t Use specified device instead of the sound "
	    "service. Use location path or a special device `default'\n");
}

int main(int argc, char *argv[])
{
	const char *device = "default";
	int idx = 0;
	bool direct = false, record = false;
	optind = 0;
	int ret = 0;
	while (ret != -1) {
		ret = getopt_long(argc, argv, "d:rh", opts, &idx);
		switch (ret) {
		case 'd':
			direct = true;
			device = optarg;
			break;
		case 'r':
			record = true;
			break;
		case 'h':
			print_help(*argv);
			return 0;
		};
	}

	if (optind == argc) {
		printf("Not enough arguments.\n");
		print_help(*argv);
		return 1;
	}

	for (int i = optind; i < argc; ++i) {
		const char *file = argv[i];

		printf("%s (%d/%d) %s\n", record ? "Recording" : "Playing",
		    i - optind + 1, argc - optind, file);
		if (record) {
			printf("Recording is not supported yet.\n");
			return 1;
		}
		if (direct) {
			dplay(device, file);
		} else {
			hplay(file);
		}
	}
	return 0;
}
/**
 * @}
 */
