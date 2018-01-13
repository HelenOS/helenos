/*
 * Copyright (c) 2014 Jiri Svoboda
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

/** @addtogroup edit
 * @brief Amiga music module player.
 * @{
 */
/**
 * @file
 */

#include <errno.h>
#include <hound/client.h>
#include <io/console.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>
#include <trackmod.h>

static bool quit = false;

static void modplay_key_press(kbd_event_t *ev)
{
	if ((ev->mods & KM_ALT) == 0 &&
	    (ev->mods & KM_SHIFT) == 0 &&
	    (ev->mods & KM_CTRL) != 0) {
		if (ev->key == KC_Q)
			quit = true;
	}
}

static void modplay_event(cons_event_t *event)
{
	switch (event->type) {
	case CEV_KEY:
		if (event->ev.key.type == KEY_PRESS) {
			modplay_key_press(&event->ev.key);
		}
		break;
	default:
		break;
	}
}

int main(int argc, char *argv[])
{
	trackmod_module_t *mod;
	trackmod_modplay_t *modplay;
	hound_context_t *hound;
	console_ctrl_t *con;
	cons_event_t event;
	suseconds_t timeout;
	pcm_format_t format;
	void *buffer;
	size_t buffer_size;
	errno_t rc;

	if (argc != 2) {
		printf("syntax: modplay <filename.mod>\n");
		return 1;
	}

	con = console_init(stdin, stdout);

	rc = trackmod_module_load(argv[1], &mod);
	if (rc != EOK) {
		printf("Error loading %s.\n", argv[1]);
		return 1;
	}


	format.channels = 1;
	format.sampling_rate = 44100;
#ifdef __LE__
	format.sample_format = PCM_SAMPLE_SINT16_LE;
#else
	format.sample_format = PCM_SAMPLE_SINT16_BE;
#endif
	buffer_size = 64 * 1024;

	buffer = malloc(buffer_size);
	if (buffer == NULL) {
		printf("Error allocating audio buffer.\n");
		return 1;
	}

	hound = hound_context_create_playback(argv[1], format, buffer_size);
	if (hound == NULL) {
		printf("Error creating playback context.\n");
		return 1;
	}

	rc = hound_context_connect_target(hound, HOUND_DEFAULT_TARGET);
	if (rc != EOK) {
		printf("Error connecting default audio target: %s.\n", str_error(rc));
		return 1;
	}

	rc = trackmod_modplay_create(mod, format.sampling_rate, &modplay);
	if (rc != EOK) {
		printf("Error setting up playback.\n");
		return 1;
	}

	printf("Playing '%s'. Press Ctrl+Q to quit.\n", argv[1]);

	while (true) {
		timeout = 0;
		if (console_get_event_timeout(con, &event, &timeout))
			modplay_event(&event);
		if (quit)
			break;

		trackmod_modplay_get_samples(modplay, buffer, buffer_size / 4);

		rc = hound_write_main_stream(hound, buffer, buffer_size / 4);
		if (rc != EOK) {
			printf("Error writing audio stream.\n");
			break;
		}
	}

	hound_context_destroy(hound);
	trackmod_modplay_destroy(modplay);
	trackmod_module_destroy(mod);

	return 0;
}


/** @}
 */
