/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#include <arch/i8042.h>
#include <arch/i8259.h>
#include <arch/interrupt.h>
#include <cpu.h>
#include <arch/asm.h>
#include <arch.h>
#include <print.h>
#include <synch/spinlock.h>
#include <typedefs.h>

/**
 * i8042 processor driver.
 * It takes care of low-level keyboard functions.
 */

#define SPECIAL		'?'
#define KEY_RELEASE	0x80

static void key_released(__u8 sc);
static void key_pressed(__u8 sc);

#define PRESSED_SHIFT		(1<<0)
#define PRESSED_CAPSLOCK	(1<<1)
#define LOCKED_CAPSLOCK		(1<<0)

static spinlock_t keylock;		/**< keylock protects keyflags and lockflags. */
static volatile int keyflags;		/**< Tracking of multiple keypresses. */
static volatile int lockflags;		/**< Tracking of multiple keys lockings. */

/** Primary meaning of scancodes. */
static char sc_primary_map[] = {
	SPECIAL, /* 0x00 */
	SPECIAL, /* 0x01 - Esc */
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
	SPECIAL, /* 0x0e - Backspace */
	'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
	SPECIAL, /* 0x1d - LCtrl */
	'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'',
	'`',
	SPECIAL, /* 0x2a - LShift */ 
	'\\',
	'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
	SPECIAL, /* 0x36 - RShift */
	'*',
	SPECIAL, /* 0x38 - LAlt */
	' ',
	SPECIAL, /* 0x3a - CapsLock */
	SPECIAL, /* 0x3b - F1 */
	SPECIAL, /* 0x3c - F2 */
	SPECIAL, /* 0x3d - F3 */
	SPECIAL, /* 0x3e - F4 */
	SPECIAL, /* 0x3f - F5 */
	SPECIAL, /* 0x40 - F6 */
	SPECIAL, /* 0x41 - F7 */
	SPECIAL, /* 0x42 - F8 */
	SPECIAL, /* 0x43 - F9 */
	SPECIAL, /* 0x44 - F10 */
	SPECIAL, /* 0x45 - NumLock */
	SPECIAL, /* 0x46 - ScrollLock */
	'7', '8', '9', '-',
	'4', '5', '6', '+',
	'1', '2', '3',
	'0', '.',
	SPECIAL, /* 0x54 - Alt-SysRq */
	SPECIAL, /* 0x55 - F11/F12/PF1/FN */
	SPECIAL, /* 0x56 - unlabelled key next to LAlt */
	SPECIAL, /* 0x57 - F11 */
	SPECIAL, /* 0x58 - F12 */
	SPECIAL, /* 0x59 */
	SPECIAL, /* 0x5a */
	SPECIAL, /* 0x5b */
	SPECIAL, /* 0x5c */
	SPECIAL, /* 0x5d */
	SPECIAL, /* 0x5e */
	SPECIAL, /* 0x5f */
	SPECIAL, /* 0x60 */
	SPECIAL, /* 0x61 */
	SPECIAL, /* 0x62 */
	SPECIAL, /* 0x63 */
	SPECIAL, /* 0x64 */
	SPECIAL, /* 0x65 */
	SPECIAL, /* 0x66 */
	SPECIAL, /* 0x67 */
	SPECIAL, /* 0x68 */
	SPECIAL, /* 0x69 */
	SPECIAL, /* 0x6a */
	SPECIAL, /* 0x6b */
	SPECIAL, /* 0x6c */
	SPECIAL, /* 0x6d */
	SPECIAL, /* 0x6e */
	SPECIAL, /* 0x6f */
	SPECIAL, /* 0x70 */
	SPECIAL, /* 0x71 */
	SPECIAL, /* 0x72 */
	SPECIAL, /* 0x73 */
	SPECIAL, /* 0x74 */
	SPECIAL, /* 0x75 */
	SPECIAL, /* 0x76 */
	SPECIAL, /* 0x77 */
	SPECIAL, /* 0x78 */
	SPECIAL, /* 0x79 */
	SPECIAL, /* 0x7a */
	SPECIAL, /* 0x7b */
	SPECIAL, /* 0x7c */
	SPECIAL, /* 0x7d */
	SPECIAL, /* 0x7e */
	SPECIAL, /* 0x7f */
};

/** Secondary meaning of scancodes. */
static char sc_secondary_map[] = {
	SPECIAL, /* 0x00 */
	SPECIAL, /* 0x01 - Esc */
	'!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
	SPECIAL, /* 0x0e - Backspace */
	'\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
	SPECIAL, /* 0x1d - LCtrl */
	'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"',
	'~',
	SPECIAL, /* 0x2a - LShift */ 
	'|',
	'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
	SPECIAL, /* 0x36 - RShift */
	'*',
	SPECIAL, /* 0x38 - LAlt */
	' ',
	SPECIAL, /* 0x3a - CapsLock */
	SPECIAL, /* 0x3b - F1 */
	SPECIAL, /* 0x3c - F2 */
	SPECIAL, /* 0x3d - F3 */
	SPECIAL, /* 0x3e - F4 */
	SPECIAL, /* 0x3f - F5 */
	SPECIAL, /* 0x40 - F6 */
	SPECIAL, /* 0x41 - F7 */
	SPECIAL, /* 0x42 - F8 */
	SPECIAL, /* 0x43 - F9 */
	SPECIAL, /* 0x44 - F10 */
	SPECIAL, /* 0x45 - NumLock */
	SPECIAL, /* 0x46 - ScrollLock */
	'7', '8', '9', '-',
	'4', '5', '6', '+',
	'1', '2', '3',
	'0', '.',
	SPECIAL, /* 0x54 - Alt-SysRq */
	SPECIAL, /* 0x55 - F11/F12/PF1/FN */
	SPECIAL, /* 0x56 - unlabelled key next to LAlt */
	SPECIAL, /* 0x57 - F11 */
	SPECIAL, /* 0x58 - F12 */
	SPECIAL, /* 0x59 */
	SPECIAL, /* 0x5a */
	SPECIAL, /* 0x5b */
	SPECIAL, /* 0x5c */
	SPECIAL, /* 0x5d */
	SPECIAL, /* 0x5e */
	SPECIAL, /* 0x5f */
	SPECIAL, /* 0x60 */
	SPECIAL, /* 0x61 */
	SPECIAL, /* 0x62 */
	SPECIAL, /* 0x63 */
	SPECIAL, /* 0x64 */
	SPECIAL, /* 0x65 */
	SPECIAL, /* 0x66 */
	SPECIAL, /* 0x67 */
	SPECIAL, /* 0x68 */
	SPECIAL, /* 0x69 */
	SPECIAL, /* 0x6a */
	SPECIAL, /* 0x6b */
	SPECIAL, /* 0x6c */
	SPECIAL, /* 0x6d */
	SPECIAL, /* 0x6e */
	SPECIAL, /* 0x6f */
	SPECIAL, /* 0x70 */
	SPECIAL, /* 0x71 */
	SPECIAL, /* 0x72 */
	SPECIAL, /* 0x73 */
	SPECIAL, /* 0x74 */
	SPECIAL, /* 0x75 */
	SPECIAL, /* 0x76 */
	SPECIAL, /* 0x77 */
	SPECIAL, /* 0x78 */
	SPECIAL, /* 0x79 */
	SPECIAL, /* 0x7a */
	SPECIAL, /* 0x7b */
	SPECIAL, /* 0x7c */
	SPECIAL, /* 0x7d */
	SPECIAL, /* 0x7e */
	SPECIAL, /* 0x7f */	
};

/** Initialize i8042. */
void i8042_init(void)
{
	trap_register(VECTOR_KBD, i8042_interrupt);
	spinlock_initialize(&keylock);
}

/** Process i8042 interrupt.
 *
 * @param n Interrupt vector.
 * @param stack Interrupted stack.
 */
void i8042_interrupt(__u8 n, __native stack[])
{
	__u8 x;

	trap_virtual_eoi();
	x = inb(0x60);
	if (x & KEY_RELEASE)
		key_released(x ^ KEY_RELEASE);
	else
		key_pressed(x);
}

/** Process release of key.
 *
 * @param sc Scancode of the key being released.
 */
void key_released(__u8 sc)
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
void key_pressed(__u8 sc)
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
	    default:
	    	letter = (ascii >= 'a') && (ascii <= 'z');
		capslock = (keyflags & PRESSED_CAPSLOCK) || (lockflags & LOCKED_CAPSLOCK);
		shift = keyflags & PRESSED_SHIFT;
		if (letter && capslock)
			shift = !shift;
		if (shift)
			map = sc_secondary_map;
		putchar(map[sc]);
		break;
	}
	spinlock_unlock(&keylock);
}
