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
#include <loc.h>
#include <str_error.h>
#include <str.h>
#include <audio_mixer_iface.h>
#include <stdio.h>

#define DEFAULT_SERVICE "devices/\\hw\\pci0\\00:01.0\\sb16\\control"

/**
 * Print volume levels on all channels on all control items.
 * @param exch IPC exchange
 */
static void print_levels(async_exch_t *exch)
{
	char* name = NULL;
	unsigned count = 0;
	errno_t ret = audio_mixer_get_info(exch, &name, &count);
	if (ret != EOK) {
		printf("Failed to get mixer info: %s.\n", str_error(ret));
		return;
	}
	printf("MIXER %s:\n\n", name);

	for (unsigned i = 0; i < count; ++i) {
		char *name = NULL;
		unsigned levels = 0, current = 0;
		errno_t ret =
		    audio_mixer_get_item_info(exch, i, &name, &levels);
		if (ret != EOK) {
			printf("Failed to get item %u info: %s.\n",
			    i, str_error(ret));
			continue;
		}
		ret = audio_mixer_get_item_level(exch, i, &current);
		if (ret != EOK) {
			printf("Failed to get item %u info: %s.\n",
			    i, str_error(ret));
			continue;
		}

		printf("Control item %u `%s' : %u/%u.\n",
		    i, name, current, levels - 1);
		free(name);

	}
}

static unsigned get_number(const char* str)
{
	uint16_t num;
	str_uint16_t(str, NULL, 10, false, &num);
	return num;
}

static void set_level(async_exch_t *exch, int argc, char *argv[])
{
	assert(exch);
	if (argc != 4 && argc != 5) {
		printf("%s [device] setlevel item value\n", argv[0]);
		return;
	}
	unsigned params = argc == 5 ? 3 : 2;
	const unsigned item = get_number(argv[params++]);
	const unsigned value = get_number(argv[params]);
	errno_t ret = audio_mixer_set_item_level(exch, item, value);
	if (ret != EOK) {
		printf("Failed to set item level: %s.\n", str_error(ret));
		return;
	}
	printf("Control item %u new level is %u.\n", item, value);
}

static void get_level(async_exch_t *exch, int argc, char *argv[])
{
	assert(exch);
	if (argc != 3 && argc != 4) {
		printf("%s [device] getlevel item \n", argv[0]);
		return;
	}
	unsigned params = argc == 4 ? 3 : 2;
	const unsigned item = get_number(argv[params++]);
	unsigned value = 0;

	errno_t ret = audio_mixer_get_item_level(exch, item, &value);
	if (ret != EOK) {
		printf("Failed to get item level: %s.\n", str_error(ret));
		return;
	}
	printf("Control item %u level: %u.\n", item, value);
}

int main(int argc, char *argv[])
{
	const char *service = DEFAULT_SERVICE;
	void (*command)(async_exch_t *, int, char*[]) = NULL;

	if (argc >= 2 && str_cmp(argv[1], "setlevel") == 0) {
		command = set_level;
		if (argc == 5)
			service = argv[1];
	}

	if (argc >= 2 && str_cmp(argv[1], "getlevel") == 0) {
		command = get_level;
		if (argc == 4)
			service = argv[1];
	}

	if ((argc == 2 && command == NULL))
		service = argv[1];


	service_id_t mixer_sid;
	errno_t rc = loc_service_get_id(service, &mixer_sid, 0);
	if (rc != EOK) {
		printf("Failed to resolve service '%s': %s.\n",
		    service, str_error(rc));
		return 1;
	}

	async_sess_t *session = loc_service_connect(mixer_sid, INTERFACE_DDF, 0);
	if (!session) {
		printf("Failed connecting mixer service '%s'.\n", service);
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
		printf("\n%s:\n", argv[0]);
		printf("Use '%s getlevel idx' command to read individual "
		    "settings\n", argv[0]);
		printf("Use '%s setlevel idx' command to change "
		    "settings\n", argv[0]);
	}

	async_exchange_end(exch);
	async_hangup(session);
	return 0;
}

/** @}
 */
