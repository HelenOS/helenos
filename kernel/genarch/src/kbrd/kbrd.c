/*
 * Copyright (c) 2009 Jakub Jermar
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

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief Keyboard processing.
 */

#include <assert.h>
#include <genarch/kbrd/kbrd.h>
#include <genarch/kbrd/scanc.h>

#ifdef CONFIG_PC_KBD
#include <genarch/kbrd/scanc_pc.h>
#endif

#ifdef CONFIG_SUN_KBD
#include <genarch/kbrd/scanc_sun.h>
#endif

#ifdef CONFIG_MAC_KBD
#include <genarch/kbrd/scanc_mac.h>
#endif

#include <synch/spinlock.h>
#include <console/chardev.h>
#include <console/console.h>
#include <proc/thread.h>
#include <arch.h>
#include <macros.h>
#include <str.h>
#include <stdlib.h>

#define IGNORE_CODE  0x7f
#define KEY_RELEASE  0x80

#define PRESSED_SHIFT     (1 << 0)
#define PRESSED_CAPSLOCK  (1 << 1)
#define LOCKED_CAPSLOCK   (1 << 0)

static indev_operations_t kbrd_raw_ops = {
	.poll = NULL,
	.signal = NULL
};

/** Process release of key.
 *
 * @param sc Scancode of the key being released.
 */
static void key_released(kbrd_instance_t *instance, char32_t sc)
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
static void key_pressed(kbrd_instance_t *instance, char32_t sc)
{
	bool letter;
	bool shift;
	bool capslock;
	char32_t ch;

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
			ch = sc_secondary_map[sc];
		else
			ch = sc_primary_map[sc];

		switch (ch) {
		case U_PAGE_UP:
			indev_signal(instance->sink, INDEV_SIGNAL_SCROLL_UP);
			break;
		case U_PAGE_DOWN:
			indev_signal(instance->sink, INDEV_SIGNAL_SCROLL_DOWN);
			break;
		default:
			indev_push_character(instance->sink, ch);
		}

		break;
	}

	spinlock_unlock(&instance->keylock);
}

static void kkbrd(void *arg)
{
	kbrd_instance_t *instance = (kbrd_instance_t *) arg;

	while (true) {
		char32_t sc = indev_pop_character(&instance->raw);

		if (sc == IGNORE_CODE)
			continue;

		if (sc & KEY_RELEASE)
			key_released(instance, (sc ^ KEY_RELEASE) & 0x7f);
		else
			key_pressed(instance, sc & 0x7f);
	}
}

kbrd_instance_t *kbrd_init(void)
{
	kbrd_instance_t *instance =
	    malloc(sizeof(kbrd_instance_t));
	if (instance) {
		instance->thread = thread_create(kkbrd, (void *) instance,
		    TASK, THREAD_FLAG_NONE, "kkbrd");

		if (!instance->thread) {
			free(instance);
			return NULL;
		}

		instance->sink = NULL;
		indev_initialize("kbrd", &instance->raw, &kbrd_raw_ops);

		spinlock_initialize(&instance->keylock, "kbrd.instance.keylock");
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
	thread_start(instance->thread);

	return &instance->raw;
}

/** @}
 */
