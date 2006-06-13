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

 /** @addtogroup genarch	
 * @{
 */
/** @file
 */

#include <genarch/i8042/i8042.h>
#include <arch/drivers/i8042.h>
#include <arch/interrupt.h>
#include <cpu.h>
#include <arch/asm.h>
#include <arch.h>
#include <synch/spinlock.h>
#include <typedefs.h>
#include <console/chardev.h>
#include <console/console.h>
#include <macros.h>
#include <interrupt.h>

/**
 * i8042 processor driver.
 * It takes care of low-level keyboard functions.
 */

/** Keyboard commands. */
#define KBD_ENABLE	0xf4
#define KBD_DISABLE	0xf5
#define KBD_ACK		0xfa

/*
 * 60  Write 8042 Command Byte: next data byte written to port 60h is
 *     placed in 8042 command register. Format:
 *
 *    |7|6|5|4|3|2|1|0|8042 Command Byte
 *     | | | | | | | `---- 1=enable output register full interrupt
 *     | | | | | | `----- should be 0
 *     | | | | | `------ 1=set status register system, 0=clear
 *     | | | | `------- 1=override keyboard inhibit, 0=allow inhibit
 *     | | | `-------- disable keyboard I/O by driving clock line low
 *     | | `--------- disable auxiliary device, drives clock line low
 *     | `---------- IBM scancode translation 0=AT, 1=PC/XT
 *     `----------- reserved, should be 0
 */

#define i8042_SET_COMMAND 	0x60
#define i8042_COMMAND 		0x49

#define i8042_BUFFER_FULL_MASK	0x01
#define i8042_WAIT_MASK 	0x02

#define SPECIAL		'?'
#define KEY_RELEASE	0x80

/**
 * These codes read from i8042 data register are silently ignored.
 */
#define IGNORE_CODE	0x7f

static void key_released(__u8 sc);
static void key_pressed(__u8 sc);
static char key_read(chardev_t *d);

#define PRESSED_SHIFT		(1<<0)
#define PRESSED_CAPSLOCK	(1<<1)
#define LOCKED_CAPSLOCK		(1<<0)

#define ACTIVE_READ_BUFF_SIZE 16 	/* Must be power of 2 */

static __u8 active_read_buff[ACTIVE_READ_BUFF_SIZE];

SPINLOCK_INITIALIZE(keylock);		/**< keylock protects keyflags and lockflags. */
static volatile int keyflags;		/**< Tracking of multiple keypresses. */
static volatile int lockflags;		/**< Tracking of multiple keys lockings. */

static void i8042_suspend(chardev_t *);
static void i8042_resume(chardev_t *);

static chardev_t kbrd;
static chardev_operations_t ops = {
	.suspend = i8042_suspend,
	.resume = i8042_resume,
	.read = key_read
};

/** Primary meaning of scancodes. */
static char sc_primary_map[] = {
	SPECIAL, /* 0x00 */
	SPECIAL, /* 0x01 - Esc */
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
	'\b', /* 0x0e - Backspace */
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

static void i8042_interrupt(int n, istate_t *istate);
static void i8042_wait(void);

static iroutine oldvector;
/** Initialize keyboard and service interrupts using kernel routine */
void i8042_grab(void)
{
	oldvector = exc_register(VECTOR_KBD, "i8042_interrupt", (iroutine) i8042_interrupt);
	i8042_wait();
	i8042_command_write(i8042_SET_COMMAND);
	i8042_wait();
	i8042_data_write(i8042_COMMAND);
	i8042_wait();
}
/** Resume the former interrupt vector */
void i8042_release(void)
{
	if (oldvector)
		exc_register(VECTOR_KBD, "user_interrupt", oldvector);
}

/** Initialize i8042. */
void i8042_init(void)
{
	int i;

	i8042_grab();
        /* Prevent user from accidentaly releasing calling i8042_resume
	 * and disabling keyboard 
	 */
	oldvector = NULL; 

	trap_virtual_enable_irqs(1<<IRQ_KBD);
	chardev_initialize("i8042_kbd", &kbrd, &ops);
	stdin = &kbrd;

	/*
	 * Clear input buffer.
	 * Number of iterations is limited to prevent infinite looping.
	 */
	for (i = 0; (i8042_status_read() & i8042_BUFFER_FULL_MASK) && i < 100; i++) {
		i8042_data_read();
	}  
}

/** Process i8042 interrupt.
 *
 * @param n Interrupt vector.
 * @param stack Interrupted stack.
 */
void i8042_interrupt(int n, istate_t *istate)
{
	__u8 x;

	trap_virtual_eoi();
	x = i8042_data_read();
	if (x & KEY_RELEASE)
		key_released(x ^ KEY_RELEASE);
	else
		key_pressed(x);
}

/** Wait until the controller reads its data. */
void i8042_wait(void) {
	while (i8042_status_read() & i8042_WAIT_MASK) {
		/* wait */
	}
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
	    	letter = is_lower(ascii);
		capslock = (keyflags & PRESSED_CAPSLOCK) || (lockflags & LOCKED_CAPSLOCK);
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

/* Called from getc(). */
void i8042_resume(chardev_t *d)
{
}

/* Called from getc(). */
void i8042_suspend(chardev_t *d)
{
}

static __u8 active_read_buff_read(void)
{
	static int i=0;
	i &= (ACTIVE_READ_BUFF_SIZE-1);
	if(!active_read_buff[i]) {
		return 0;
	}
	return active_read_buff[i++];
}

static void active_read_buff_write(__u8 ch)
{
	static int i=0;
	active_read_buff[i] = ch;
	i++;
	i &= (ACTIVE_READ_BUFF_SIZE-1);
	active_read_buff[i]=0;
}


static void active_read_key_pressed(__u8 sc)
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
	    	letter = is_lower(ascii);
		capslock = (keyflags & PRESSED_CAPSLOCK) || (lockflags & LOCKED_CAPSLOCK);
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

static char key_read(chardev_t *d)
{
	char ch;	

	while(!(ch = active_read_buff_read())) {
		__u8 x;
		while (!((x=i8042_status_read() & i8042_BUFFER_FULL_MASK)))
			;
		x = i8042_data_read();
		if (x != IGNORE_CODE) {
			if (x & KEY_RELEASE)
				key_released(x ^ KEY_RELEASE);
			else
				active_read_key_pressed(x);
		}
	}
	return ch;
}

/** Poll for key press and release events.
 *
 * This function can be used to implement keyboard polling.
 */
void i8042_poll(void)
{
	__u8 x;

	while (((x = i8042_status_read() & i8042_BUFFER_FULL_MASK))) {
		x = i8042_data_read();
		if (x != IGNORE_CODE) {
			if (x & KEY_RELEASE)
				key_released(x ^ KEY_RELEASE);
			else
				key_pressed(x);
		}
	}
}

 /** @}
 */

