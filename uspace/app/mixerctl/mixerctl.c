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

/** @addtogroup mixerctl
 * @{
 */
/** @file Mixer control for audio devices
 */

#include <assert.h>
#include <errno.h>
#include <str_error.h>
#include <str.h>
#include <devman.h>
#include <audio_mixer_iface.h>
#include <stdio.h>

#define DEFAULT_DEVICE "/hw/pci0/00:01.0/sb16/mixer"

static void print_levels(async_exch_t *exch)
{
	const char* name = NULL;
	unsigned count = 0;
	int ret = audio_mixer_get_info(exch, &name, &count);
	if (ret != EOK) {
		printf("Failed to get mixer info: %s.\n", str_error(ret));
		return;
	}
	printf("MIXER %s:\n", name);

	for (unsigned i = 0; i < count; ++i) {
		const char *name = NULL;
		unsigned channels = 0;
		const int ret =
		    audio_mixer_get_item_info(exch, i, &name, &channels);
		if (ret != EOK) {
			printf("Failed to get item %u info: %s.\n",
			    i, str_error(ret));
			continue;
		}
		for (unsigned j = 0; j < channels; ++j) {
			const char *chan = NULL;
			int ret = audio_mixer_get_channel_info(
			    exch, i, j, &chan, NULL);
			if (ret != EOK) {
				printf(
				    "Failed to get channel %u-%u info: %s.\n",
				    i, j, str_error(ret));
			}
			unsigned level = 0, max = 0;
			ret = audio_mixer_channel_volume_get(
			    exch, i, j, &level, &max);
			if (ret != EOK) {
				printf("Failed to get channel %u-%u volume:"
				    " %s.\n", i, j, str_error(ret));
			}
			bool mute = false;
			ret = audio_mixer_channel_mute_get(
			    exch, i, j, &mute);
			if (ret != EOK) {
				printf("Failed to get channel %u-%u mute"
				    " status: %s.\n", i, j, str_error(ret));
			}

			printf("\tChannel(%u/%u) %s %s volume: %u/%u%s.\n",
			    i, j, name, chan, level, max, mute ? " (M)":"");
			free(chan);
		}
		free(name);

	}
}
/*----------------------------------------------------------------------------*/
static unsigned get_number(const char* str)
{
	uint16_t num;
	str_uint16_t(str, NULL, 10, false, &num);
	return num;
}
/*----------------------------------------------------------------------------*/
static void set_volume(async_exch_t *exch, int argc, char *argv[])
{
	assert(exch);
	if (argc != 5 && argc != 6) {
		printf("%s [device] setvolume item channel value\n", argv[0]);
	}
	unsigned params = argc == 6 ? 3 : 2;
	const unsigned item = get_number(argv[params++]);
	const unsigned channel = get_number(argv[params++]);
	const unsigned value = get_number(argv[params]);
	int ret = audio_mixer_channel_volume_set(exch, item, channel, value);
	if (ret != EOK) {
		printf("Failed to set mixer volume: %s.\n", str_error(ret));
		return;
	}
	printf("Channel %u-%u volume set to %u.\n", item, channel, value);
}
/*----------------------------------------------------------------------------*/
static void get_volume(async_exch_t *exch, int argc, char *argv[])
{
	assert(exch);
	if (argc != 4 && argc != 5) {
		printf("%s [device] getvolume item channel\n", argv[0]);
	}
	unsigned params = argc == 5 ? 3 : 2;
	const unsigned item = get_number(argv[params++]);
	const unsigned channel = get_number(argv[params++]);
	unsigned value = 0, max = 0;

	int ret = audio_mixer_channel_volume_get(
	    exch, item, channel, &value, &max);
	if (ret != EOK) {
		printf("Failed to get mixer volume: %s.\n", str_error(ret));
		return;
	}
	printf("Channel %u-%u volume: %u/%u.\n", item, channel, value, max);
}
/*----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	const char *device = DEFAULT_DEVICE;
	void (*command)(async_exch_t *, int, char*[]) = NULL;

	if (argc >= 2 && str_cmp(argv[1], "setvolume") == 0) {
		command = set_volume;
		if (argc == 6)
			device = argv[1];
	}

	if (argc >= 2 && str_cmp(argv[1], "getvolume") == 0) {
		command = get_volume;
		if (argc == 5)
			device = argv[1];
	}

	if ((argc == 2 && command == NULL))
		device = argv[1];


	devman_handle_t mixer_handle;
	int ret = devman_fun_get_handle(device, &mixer_handle, 0);
	if (ret != EOK) {
		printf("Failed to get device(%s) handle: %s.\n",
		    device, str_error(ret));
		return 1;
	}

	async_sess_t *session = devman_device_connect(
	    EXCHANGE_ATOMIC, mixer_handle, IPC_FLAG_BLOCKING);
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

	if (command) {
		command(exch, argc, argv);
	} else {
		print_levels(exch);
	}

	async_exchange_end(exch);
	async_hangup(session);
	return 0;
}

/** @}
 */
