/*
 * Copyright (c) 2006 Jakub Jermar
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
 * @brief	Key processing.
 */

#include <genarch/kbd/key.h>
#include <genarch/kbd/scanc.h>
#ifdef CONFIG_I8042
#include <genarch/kbd/scanc_pc.h>
#endif

#if (defined(sparc64))
#include <genarch/kbd/scanc_sun.h>
#endif

#include <synch/spinlock.h>
#include <console/chardev.h>
#include <macros.h>

#define PRESSED_SHIFT		(1<<0)
#define PRESSED_CAPSLOCK	(1<<1)
#define LOCKED_CAPSLOCK		(1<<0)

#define ACTIVE_READ_BUFF_SIZE 16 	/* Must be power of 2 */

chardev_t kbrd;

static uint8_t active_read_buff[ACTIVE_READ_BUFF_SIZE];

SPINLOCK_INITIALIZE(keylock);		/**< keylock protects keyflags and lockflags. */
static volatile int keyflags;		/**< Tracking of multiple keypresses. */
static volatile int lockflags;		/**< Tracking of multiple keys lockings. */

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
		chardev_push_character(&kbrd, 0x1b);
		chardev_push_character(&kbrd, 0x5b);
		chardev_push_character(&kbrd, 0x44);
		break;
	case SC_RIGHTARR:
		chardev_push_character(&kbrd, 0x1b);
		chardev_push_character(&kbrd, 0x5b);
		chardev_push_character(&kbrd, 0x43);
		break;
	case SC_UPARR:
		chardev_push_character(&kbrd, 0x1b);
		chardev_push_character(&kbrd, 0x5b);
		chardev_push_character(&kbrd, 0x41);
		break;
	case SC_DOWNARR:
		chardev_push_character(&kbrd, 0x1b);
		chardev_push_character(&kbrd, 0x5b);
		chardev_push_character(&kbrd, 0x42);
		break;
	case SC_HOME:
		chardev_push_character(&kbrd, 0x1b);
		chardev_push_character(&kbrd, 0x4f);
		chardev_push_character(&kbrd, 0x48);
		break;
	case SC_END:
		chardev_push_character(&kbrd, 0x1b);
		chardev_push_character(&kbrd, 0x4f);
		chardev_push_character(&kbrd, 0x46);
		break;
	case SC_DELETE:
		chardev_push_character(&kbrd, 0x1b);
		chardev_push_character(&kbrd, 0x5b);
		chardev_push_character(&kbrd, 0x33);
		chardev_push_character(&kbrd, 0x7e);
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
		chardev_push_character(&kbrd, map[sc]);
		break;
	}
	spinlock_unlock(&keylock);
}

uint8_t active_read_buff_read(void)
{
	static int i=0;
	i &= (ACTIVE_READ_BUFF_SIZE-1);
	if(!active_read_buff[i]) {
		return 0;
	}
	return active_read_buff[i++];
}

void active_read_buff_write(uint8_t ch)
{
	static int i=0;
	active_read_buff[i] = ch;
	i++;
	i &= (ACTIVE_READ_BUFF_SIZE-1);
	active_read_buff[i]=0;
}


void active_read_key_pressed(uint8_t sc)
{
	char *map = sc_primary_map;
	char ascii = sc_primary_map[sc];
	bool shift, capslock;
	bool letter = false;

	/*spinlock_lock(&keylock);*/
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
		active_read_buff_write(0x1b);
		active_read_buff_write(0x5b);
		active_read_buff_write(0x44);
		break;
	case SC_RIGHTARR:
		active_read_buff_write(0x1b);
		active_read_buff_write(0x5b);
		active_read_buff_write(0x43);
		break;
	case SC_UPARR:
		active_read_buff_write(0x1b);
		active_read_buff_write(0x5b);
		active_read_buff_write(0x41);
		break;
	case SC_DOWNARR:
		active_read_buff_write(0x1b);
		active_read_buff_write(0x5b);
		active_read_buff_write(0x42);
		break;
	case SC_HOME:
		active_read_buff_write(0x1b);
		active_read_buff_write(0x4f);
		active_read_buff_write(0x48);
		break;
	case SC_END:
		active_read_buff_write(0x1b);
		active_read_buff_write(0x4f);
		active_read_buff_write(0x46);
		break;
	case SC_DELETE:
		active_read_buff_write(0x1b);
		active_read_buff_write(0x5b);
		active_read_buff_write(0x33);
		active_read_buff_write(0x7e);
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
		active_read_buff_write(map[sc]);
		break;
	}
	/*spinlock_unlock(&keylock);*/

}

/** @}
 */
