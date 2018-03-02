/*
 * Copyright (c) 2009 Vineeth Pillai
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

/** @addtogroup genarch
 * @{
 */
/**
 * @file
 * @brief PC/AT Keyboard processing.
 */

#include <assert.h>
#include <genarch/kbrd/kbrd.h>
#include <genarch/kbrd/scanc.h>

#include <genarch/kbrd/scanc_at.h>

#include <synch/spinlock.h>
#include <console/chardev.h>
#include <console/console.h>
#include <proc/thread.h>
#include <arch.h>
#include <macros.h>

#define PRESSED_SHIFT     (1 << 0)
#define PRESSED_CAPSLOCK  (1 << 1)
#define LOCKED_CAPSLOCK   (1 << 0)

#define AT_KEY_RELEASE		0xF0
#define AT_ESC_KEY		0xE0
#define AT_CAPS_SCAN_CODE	0x58
#define AT_NUM_SCAN_CODE	0x77
#define AT_SCROLL_SCAN_CODE	0x7E

static bool is_lock_key(wchar_t);

static indev_operations_t kbrd_raw_ops = {
	.poll = NULL
};

/** Process release of key.
 *
 * @param sc Scancode of the key being released.
 */
static void key_released(kbrd_instance_t *instance, wchar_t sc)
{
	spinlock_lock(&instance->keylock);

	switch (sc) {
	case SC_LSHIFT:
	case SC_RSHIFT:
		instance->keyflags &= ~PRESSED_SHIFT;
		break;
	case SC_CAPSLOCK:
		instance->keyflags &= ~PRESSED_CAPSLOCK;
		if (instance->lockflags & LOCKED_CAPSLOCK)
			instance->lockflags &= ~LOCKED_CAPSLOCK;
		else
			instance->lockflags |= LOCKED_CAPSLOCK;
		break;
	default:
		break;
	}

	spinlock_unlock(&instance->keylock);
}

/** Process keypress.
 *
 * @param sc Scancode of the key being pressed.
 */
static void key_pressed(kbrd_instance_t *instance, wchar_t sc)
{
	bool letter;
	bool shift;
	bool capslock;

	spinlock_lock(&instance->keylock);

	switch (sc) {
	case SC_LSHIFT:
	case SC_RSHIFT:
		instance->keyflags |= PRESSED_SHIFT;
		break;
	case SC_CAPSLOCK:
		instance->keyflags |= PRESSED_CAPSLOCK;
		break;
	case SC_SCAN_ESCAPE:
		break;
	default:
		letter = islower(sc_primary_map[sc]);
		shift = instance->keyflags & PRESSED_SHIFT;
		capslock = (instance->keyflags & PRESSED_CAPSLOCK) ||
		    (instance->lockflags & LOCKED_CAPSLOCK);

		if ((letter) && (capslock))
			shift = !shift;

		if (shift)
			indev_push_character(instance->sink, sc_secondary_map[sc]);
		else
			indev_push_character(instance->sink, sc_primary_map[sc]);
		break;
	}

	spinlock_unlock(&instance->keylock);
}

static void kkbrd(void *arg)
{
	static int key_released_flag = 0;
	static int is_locked = 0;
	kbrd_instance_t *instance = (kbrd_instance_t *) arg;

	while (true) {
		wchar_t sc = indev_pop_character(&instance->raw);

		if (sc == AT_KEY_RELEASE) {
			key_released_flag = 1;
		} else {
			if (key_released_flag) {
				key_released_flag = 0;
				if (is_lock_key(sc)) {
					if (!is_locked) {
						is_locked = 1;
					} else {
						is_locked = 0;
						continue;
					}
				}
				key_released(instance, sc);

			} else {
				if (is_lock_key(sc) && is_locked)
					continue;
				key_pressed(instance, sc);
			}
		}

	}
}

kbrd_instance_t *kbrd_init(void)
{
	kbrd_instance_t *instance;

	instance = malloc(sizeof(kbrd_instance_t), FRAME_ATOMIC);
	if (instance) {
		instance->thread = thread_create(kkbrd, (void *) instance, TASK, 0,
		    "kkbrd");

		if (!instance->thread) {
			free(instance);
			return NULL;
		}

		instance->sink = NULL;
		indev_initialize("kbrd", &instance->raw, &kbrd_raw_ops);

		spinlock_initialize(&instance->keylock, "kbrd_at.instance.keylock");
		instance->keyflags = 0;
		instance->lockflags = 0;
	}

	return instance;
}

indev_t *kbrd_wire(kbrd_instance_t *instance, indev_t *sink)
{
	assert(instance);
	assert(sink);

	instance->sink = sink;
	thread_ready(instance->thread);

	return &instance->raw;
}

static bool is_lock_key(wchar_t sc)
{
	return ((sc == AT_CAPS_SCAN_CODE) || (sc == AT_NUM_SCAN_CODE) ||
	    (sc == AT_SCROLL_SCAN_CODE));
}

/** @}
 */
