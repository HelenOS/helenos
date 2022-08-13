/*
 * SPDX-FileCopyrightText: 2011 Lubos Slovak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbhid
 * @{
 */
/**
 * @file
 * USB HID keyboard autorepeat facilities
 */

#include <fibril_synch.h>
#include <io/keycode.h>
#include <io/console.h>
#include <errno.h>

#include <usb/debug.h>

#include "kbdrepeat.h"
#include "kbddev.h"

/**
 * Main loop handling the auto-repeat of keys.
 *
 * This functions periodically checks if there is some key to be auto-repeated.
 *
 * If a new key is to be repeated, it uses the delay before first repeat stored
 * in the keyboard structure to wait until the key has to start repeating.
 *
 * If the same key is still pressed, it uses the delay between repeats stored
 * in the keyboard structure to wait until the key should be repeated.
 *
 * If the currently repeated key is not pressed any more (
 * usb_kbd_repeat_stop() was called), it stops repeating it and starts
 * checking again.
 *
 * @note For accessing the keyboard device auto-repeat information a fibril
 *       mutex (repeat_mtx) from the @a kbd structure is used.
 *
 * @param kbd Keyboard device structure.
 */
static void usb_kbd_repeat_loop(usb_kbd_t *kbd)
{
	unsigned int delay = 0;

	usb_log_debug("Starting autorepeat loop.");

	while (true) {
		/* Check if the kbd structure is usable. */
		if (!usb_kbd_is_initialized(kbd)) {
			usb_log_warning("kbd not ready, exiting autorepeat.");
			return;
		}

		fibril_mutex_lock(&kbd->repeat_mtx);

		if (kbd->repeat.key_new > 0) {
			if (kbd->repeat.key_new == kbd->repeat.key_repeated) {
				usb_log_debug2("Repeating key: %u.",
				    kbd->repeat.key_repeated);
				usb_kbd_push_ev(kbd, KEY_PRESS,
				    kbd->repeat.key_repeated);
				delay = kbd->repeat.delay_between;
			} else {
				usb_log_debug2("New key to repeat: %u.",
				    kbd->repeat.key_new);
				kbd->repeat.key_repeated = kbd->repeat.key_new;
				delay = kbd->repeat.delay_before;
			}
		} else {
			if (kbd->repeat.key_repeated > 0) {
				usb_log_debug2("Stopping to repeat key: %u.",
				    kbd->repeat.key_repeated);
				kbd->repeat.key_repeated = 0;
			}
			delay = CHECK_DELAY;
		}
		fibril_mutex_unlock(&kbd->repeat_mtx);
		fibril_usleep(delay);
	}
}

/**
 * Main routine to be executed by a fibril for handling auto-repeat.
 *
 * Starts the loop for checking changes in auto-repeat.
 *
 * @param arg User-specified argument. Expects pointer to the keyboard device
 *            structure representing the keyboard.
 *
 * @retval EOK if the routine has finished.
 * @retval EINVAL if no argument is supplied.
 */
errno_t usb_kbd_repeat_fibril(void *arg)
{
	usb_log_debug("Autorepeat fibril spawned.");

	if (arg == NULL) {
		usb_log_error("No device!");
		return EINVAL;
	}

	usb_kbd_t *kbd = arg;

	usb_kbd_repeat_loop(kbd);

	return EOK;
}

/**
 * Start repeating particular key.
 *
 * @note Only one key is repeated at any time, so calling this function
 *       effectively cancels auto-repeat of the current repeated key (if any)
 *       and 'schedules' another key for auto-repeat.
 *
 * @param kbd Keyboard device structure.
 * @param key Key to start repeating.
 */
void usb_kbd_repeat_start(usb_kbd_t *kbd, unsigned int key)
{
	fibril_mutex_lock(&kbd->repeat_mtx);
	kbd->repeat.key_new = key;
	fibril_mutex_unlock(&kbd->repeat_mtx);
}

/**
 * Stop repeating particular key.
 *
 * @note Only one key is repeated at any time, but this function may be called
 *       even with key that is not currently repeated (in that case nothing
 *       happens).
 *
 * @param kbd Keyboard device structure.
 * @param key Key to stop repeating.
 */
void usb_kbd_repeat_stop(usb_kbd_t *kbd, unsigned int key)
{
	fibril_mutex_lock(&kbd->repeat_mtx);
	if (key == kbd->repeat.key_new) {
		kbd->repeat.key_new = 0;
	}
	fibril_mutex_unlock(&kbd->repeat_mtx);
}
/**
 * @}
 */
