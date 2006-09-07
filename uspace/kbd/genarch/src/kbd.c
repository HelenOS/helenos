/*
 * Copyright (C) 2001-2004 Jakub Jermar
 * Copyright (C) 2006 Josef Cejka
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

/** @addtogroup kbd
 * @brief	Handling of keyboard IRQ notifications for several architectures.
 * @ingroup  kbd
 * @{
 */ 
/** @file
 */

#include <key_buffer.h>
#include <arch/scanc.h>
#include <genarch/scanc.h>
#include <genarch/kbd.h>
#include <libc.h>

#define PRESSED_SHIFT		(1<<0)
#define PRESSED_CAPSLOCK	(1<<1)
#define LOCKED_CAPSLOCK		(1<<0)

static volatile int keyflags;		/**< Tracking of multiple keypresses. */
static volatile int lockflags;		/**< Tracking of multiple keys lockings. */

void key_released(keybuffer_t *keybuffer, unsigned char key)
{
	switch (key) {
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
}

void key_pressed(keybuffer_t *keybuffer, unsigned char key)
{
	int *map = sc_primary_map;
	int ascii = sc_primary_map[key];
	int shift, capslock;
	int letter = 0;

	static int esc_count = 0;

	if (key == SC_ESC) {
		esc_count++;
		if (esc_count == 3)
			__SYSCALL0(SYS_DEBUG_ENABLE_CONSOLE);
	} else {
		esc_count = 0;
	}
	
	switch (key) {
	case SC_LSHIFT:
	case SC_RSHIFT:
	    	keyflags |= PRESSED_SHIFT;
		break;
	case SC_CAPSLOCK:
		keyflags |= PRESSED_CAPSLOCK;
		break;
	case SC_SPEC_ESCAPE:
		break;
	default:
	    	letter = ((ascii >= 'a') && (ascii <= 'z'));
		capslock = (keyflags & PRESSED_CAPSLOCK) || (lockflags & LOCKED_CAPSLOCK);
		shift = keyflags & PRESSED_SHIFT;
		if (letter && capslock)
			shift = !shift;
		if (shift)
			map = sc_secondary_map;
		if (map[key] != SPECIAL)
			keybuffer_push(keybuffer, map[key]);	
		break;
	}
}

/**
 * @}
 */ 
