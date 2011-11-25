/*
 * Copyright (c) 2011 Jiri Svoboda
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

#include <errno.h>
#include <str_error.h>
#include <devman.h>
#include <audio_mixer_iface.h>
#include <stdio.h>

#define DEFAULT_DEVICE "/hw/pci0/00:01.0/sb16/mixer"

int main(int argc, char *argv[])
{
	devman_handle_t mixer_handle;
	int ret = devman_fun_get_handle(DEFAULT_DEVICE, &mixer_handle, 0);
	if (ret != EOK) {
		printf("Failed to get device(%s) handle: %s.\n",
		    DEFAULT_DEVICE, str_error(ret));
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

	const char* name = NULL;
	unsigned count = 0;
	ret = audio_mixer_get_info(exch, &name, &count);
	if (ret != EOK) {
		printf("Failed to get mixer info: %s.\n", str_error(ret));
		return 1;
	}
	printf("MIXER: %s.\n", name);

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

			printf("Channel(%u/%u) %s %s volume: %u/%u%s.\n",
			    i, j, name, chan, level, max, mute ? " (M)":"");
		}

	}


	async_exchange_end(exch);
	async_hangup(session);
	return 0;
}

/** @}
 */
