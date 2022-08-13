/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
