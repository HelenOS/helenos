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

/** @addtogroup genarch	
 * @{
 */
/**
 * @file
 * @brief	Keyboard processing.
 */

#include <genarch/kbrd/kbrd.h>
#include <genarch/kbrd/scanc.h>

#ifdef CONFIG_PC_KBD
#include <genarch/kbrd/scanc_pc.h>
#endif

#ifdef CONFIG_SUN_KBD
#include <genarch/kbrd/scanc_sun.h>
#endif

#include <synch/spinlock.h>
#include <console/chardev.h>
#include <console/console.h>
#include <proc/thread.h>
#include <arch.h>
#include <macros.h>

#ifdef CONFIG_SUN_KBD
#	define IGNORE_CODE	0x7f
#endif

#define KEY_RELEASE		0x80

#define PRESSED_SHIFT		(1 << 0)
#define PRESSED_CAPSLOCK	(1 << 1)
#define LOCKED_CAPSLOCK		(1 << 0)

chardev_t kbrdin;
static chardev_t *kbdout;

static void kbrdin_suspend(chardev_t *d)
{
}

static void kbrdin_resume(chardev_t *d)
{
}

chardev_operations_t kbrdin_ops = {
	.suspend = kbrdin_suspend,
	.resume = kbrdin_resume,
};

SPINLOCK_INITIALIZE(keylock);		/**< keylock protects keyflags and lockflags. */
static volatile int keyflags;		/**< Tracking of multiple keypresses. */
static volatile int lockflags;		/**< Tracking of multiple keys lockings. */

static void key_released(uint8_t);
static void key_pressed(uint8_t);

static void kkbrd(void *arg)
{
	chardev_t *in = (chardev_t *) arg;
	uint8_t sc;

	while (1) {
		sc = _getc(in);

#ifdef CONFIG_SUN_KBD
		if (sc == IGNORE_CODE)
			continue;
#endif
		
		if (sc & KEY_RELEASE)
			key_released(sc ^ KEY_RELEASE);
		else
			key_pressed(sc);
	}
}


void kbrd_init(chardev_t *devout)
{
	thread_t *t;

	chardev_initialize("kbrd", &kbrdin, &kbrdin_ops);
	kbdout = devout;
	
	t = thread_create(kkbrd, &kbrdin, TASK, 0, "kkbrd", false);
	ASSERT(t);
	thread_ready(t);
}

/** Process release of key.
 *
 * @param sc Scancode of the key being released.
 */
void key_released(uint8_t sc)
{
	spinlock_lock(&keylock);
	switch (sc) {
	case SC_LSHIFT:
	case SC_RSHIFT:
		keyflags &= ~PRESSED_SHIFT;
		break;
	case SC_CAPSLOCK:
		keyflags &= ~PRESSED_CAPSLOCK;
		if (lockflags & LOCKED_CAPSLOCK)
			lockflags &= ~LOCKED_CAPSLOCK;
		else
			lockflags |= LOCKED_CAPSLOCK;
		break;
	default:
		break;
	}
	spinlock_unlock(&keylock);
}

/** Process keypress.
 *
 * @param sc Scancode of the key being pressed.
 */
void key_pressed(uint8_t sc)
{
	char *map = sc_primary_map;
	char ascii = sc_primary_map[sc];
	bool shift, capslock;
	bool letter = false;

	spinlock_lock(&keylock);
	switch (sc) {
	case SC_LSHIFT:
	case SC_RSHIFT:
	    	keyflags |= PRESSED_SHIFT;
		break;
	case SC_CAPSLOCK:
		keyflags |= PRESSED_CAPSLOCK;
		break;
	case SC_SPEC_ESCAPE:
		break;
	case SC_LEFTARR:
		chardev_push_character(kbdout, 0x1b);
		chardev_push_character(kbdout, 0x5b);
		chardev_push_character(kbdout, 0x44);
		break;
	case SC_RIGHTARR:
		chardev_push_character(kbdout, 0x1b);
		chardev_push_character(kbdout, 0x5b);
		chardev_push_character(kbdout, 0x43);
		break;
	case SC_UPARR:
		chardev_push_character(kbdout, 0x1b);
		chardev_push_character(kbdout, 0x5b);
		chardev_push_character(kbdout, 0x41);
		break;
	case SC_DOWNARR:
		chardev_push_character(kbdout, 0x1b);
		chardev_push_character(kbdout, 0x5b);
		chardev_push_character(kbdout, 0x42);
		break;
	case SC_HOME:
		chardev_push_character(kbdout, 0x1b);
		chardev_push_character(kbdout, 0x4f);
		chardev_push_character(kbdout, 0x48);
		break;
	case SC_END:
		chardev_push_character(kbdout, 0x1b);
		chardev_push_character(kbdout, 0x4f);
		chardev_push_character(kbdout, 0x46);
		break;
	case SC_DELETE:
		chardev_push_character(kbdout, 0x1b);
		chardev_push_character(kbdout, 0x5b);
		chardev_push_character(kbdout, 0x33);
		chardev_push_character(kbdout, 0x7e);
		break;
	default:
	    	letter = islower(ascii);
		capslock = (keyflags & PRESSED_CAPSLOCK) ||
		    (lockflags & LOCKED_CAPSLOCK);
		shift = keyflags & PRESSED_SHIFT;
		if (letter && capslock)
			shift = !shift;
		if (shift)
			map = sc_secondary_map;
		chardev_push_character(kbdout, map[sc]);
		break;
	}
	spinlock_unlock(&keylock);
}

/** @}
 */
