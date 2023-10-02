/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** @addtogroup input
 * @{
 */
/**
 * @file
 * @brief Stroke simulator.
 *
 * When simulating a keyboard using a serial TTY we need to convert the
 * recognized strokes (such as Shift-A) to sequences of key presses and
 * releases (such as 'press Shift, press A, release A, release Shift').
 *
 */

#include <io/console.h>
#include <io/keycode.h>
#include "stroke.h"
#include "kbd.h"

/** Correspondence between modifers and the modifier keycodes. */
static unsigned int mods_keys[][2] = {
	{ KM_LALT, KC_LALT },
	{ KM_LSHIFT, KC_LSHIFT },
	{ KM_LCTRL, KC_LCTRL },
	{ 0, 0 }
};

/** Simulate keystroke using sequences of key presses and releases. */
void stroke_sim(kbd_dev_t *kdev, unsigned mod, unsigned key)
{
	int i;

	/* Simulate modifier presses. */
	i = 0;
	while (mods_keys[i][0] != 0) {
		if (mod & mods_keys[i][0]) {
			kbd_push_event(kdev, KEY_PRESS, mods_keys[i][1]);
		}
		++i;
	}

	/* Simulate key press and release. */
	if (key != 0) {
		kbd_push_event(kdev, KEY_PRESS, key);
		kbd_push_event(kdev, KEY_RELEASE, key);
	}

	/* Simulate modifier releases. */
	i = 0;
	while (mods_keys[i][0] != 0) {
		if (mod & mods_keys[i][0]) {
			kbd_push_event(kdev, KEY_RELEASE, mods_keys[i][1]);
		}
		++i;
	}
}

/**
 * @}
 */
