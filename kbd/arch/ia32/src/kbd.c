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

/** @addtogroup kbdia32 ia32
 * @brief	HelenOS ia32 / amd64 arch dependent parts of uspace keyboard handler.
 * @ingroup  kbd
 * @{
 */ 
/** @file
 * @ingroup kbdamd64
 */

#include <arch/kbd.h>
#include <ipc/ipc.h>
#include <unistd.h>
#include <kbd.h>
#include <keys.h>

/* Interesting bits for status register */
#define i8042_OUTPUT_FULL  0x1
#define i8042_INPUT_FULL   0x2
#define i8042_MOUSE_DATA   0x20

/* Command constants */
#define i8042_CMD_KBD 0x60
#define i8042_CMD_MOUSE  0xd4

/* Keyboard cmd byte */
#define i8042_KBD_IE        0x1
#define i8042_MOUSE_IE      0x2
#define i8042_KBD_DISABLE   0x10
#define i8042_MOUSE_DISABLE 0x20
#define i8042_KBD_TRANSLATE 0x40

/* Mouse constants */
#define MOUSE_OUT_INIT  0xf4
#define MOUSE_ACK       0xfa


#define SPECIAL		255
#define KEY_RELEASE	0x80

/**
 * These codes read from i8042 data register are silently ignored.
 */
#define IGNORE_CODE	0x7f

#define PRESSED_SHIFT		(1<<0)
#define PRESSED_CAPSLOCK	(1<<1)
#define LOCKED_CAPSLOCK		(1<<0)

/** Scancodes. */
#define SC_ESC		0x01
#define SC_BACKSPACE	0x0e
#define SC_LSHIFT	0x2a
#define SC_RSHIFT	0x36
#define SC_CAPSLOCK	0x3a
#define SC_SPEC_ESCAPE  0xe0
#define SC_LEFTARR      0x4b
#define SC_RIGHTARR     0x4d
#define SC_UPARR        0x48
#define SC_DOWNARR      0x50
#define SC_DELETE       0x53
#define SC_HOME         0x47
#define SC_END          0x4f

#define FUNCTION_KEYS 0x100

static volatile int keyflags;		/**< Tracking of multiple keypresses. */
static volatile int lockflags;		/**< Tracking of multiple keys lockings. */

/** Primary meaning of scancodes. */
static int sc_primary_map[] = {
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
	(FUNCTION_KEYS | 1), /* 0x3b - F1 */
	(FUNCTION_KEYS | 2), /* 0x3c - F2 */
	(FUNCTION_KEYS | 3), /* 0x3d - F3 */
	(FUNCTION_KEYS | 4), /* 0x3e - F4 */
	(FUNCTION_KEYS | 5), /* 0x3f - F5 */
	(FUNCTION_KEYS | 6), /* 0x40 - F6 */
	(FUNCTION_KEYS | 7), /* 0x41 - F7 */
	(FUNCTION_KEYS | 8), /* 0x42 - F8 */
	(FUNCTION_KEYS | 9), /* 0x43 - F9 */
	(FUNCTION_KEYS | 10), /* 0x44 - F10 */
	SPECIAL, /* 0x45 - NumLock */
	SPECIAL, /* 0x46 - ScrollLock */
	'7', '8', '9', '-',
	'4', '5', '6', '+',
	'1', '2', '3',
	'0', '.',
	SPECIAL, /* 0x54 - Alt-SysRq */
	SPECIAL, /* 0x55 - F11/F12/PF1/FN */
	SPECIAL, /* 0x56 - unlabelled key next to LAlt */
	(FUNCTION_KEYS | 11), /* 0x57 - F11 */
	(FUNCTION_KEYS | 12), /* 0x58 - F12 */
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
static int sc_secondary_map[] = {
	SPECIAL, /* 0x00 */
	0x1b, /* 0x01 - Esc */
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

irq_cmd_t i8042_cmds[2] = {
	{ CMD_PORT_READ_1, (void *)0x64, 0, 1 },
	{ CMD_PORT_READ_1, (void *)0x60, 0, 2 }
};

irq_code_t i8042_kbd = {
	2,
	i8042_cmds
};

static int key_released(keybuffer_t *keybuffer, unsigned char key)
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

static int key_pressed(keybuffer_t *keybuffer, unsigned char key)
{
	int *map = sc_primary_map;
	int ascii = sc_primary_map[key];
	int shift, capslock;
	int letter = 0;

	static int esc_count=0;

	
	if ( key == SC_ESC ) {
		esc_count++;
		if ( esc_count == 3 ) {
			__SYSCALL0(SYS_DEBUG_ENABLE_CONSOLE);
		}	
	} else {
		esc_count=0;
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
	/*	case SC_LEFTARR:
			if (keybuffer_available(keybuffer) >= 3) {
				keybuffer_push(keybuffer, 0x1b);	
				keybuffer_push(keybuffer, 0x5b);	
				keybuffer_push(keybuffer, 0x44);	
			}
			break;
		case SC_RIGHTARR:
			if (keybuffer_available(keybuffer) >= 3) {
				keybuffer_push(keybuffer, 0x1b);	
				keybuffer_push(keybuffer, 0x5b);	
				keybuffer_push(keybuffer, 0x43);	
			}
			break;
		case SC_UPARR:
			if (keybuffer_available(keybuffer) >= 3) {
				keybuffer_push(keybuffer, 0x1b);	
				keybuffer_push(keybuffer, 0x5b);	
				keybuffer_push(keybuffer, 0x41);	
			}
			break;
		case SC_DOWNARR:
			if (keybuffer_available(keybuffer) >= 3) {
				keybuffer_push(keybuffer, 0x1b);	
				keybuffer_push(keybuffer, 0x5b);	
				keybuffer_push(keybuffer, 0x42);	
			}
			break;
		case SC_HOME:
			if (keybuffer_available(keybuffer) >= 3) {
				keybuffer_push(keybuffer, 0x1b);	
				keybuffer_push(keybuffer, 0x4f);	
				keybuffer_push(keybuffer, 0x48);	
			}
			break;
		case SC_END:
			if (keybuffer_available(keybuffer) >= 3) {
				keybuffer_push(keybuffer, 0x1b);	
				keybuffer_push(keybuffer, 0x4f);	
				keybuffer_push(keybuffer, 0x46);	
			}
			break;
		case SC_DELETE:
			if (keybuffer_available(keybuffer) >= 4) {
				keybuffer_push(keybuffer, 0x1b);	
				keybuffer_push(keybuffer, 0x5b);	
				keybuffer_push(keybuffer, 0x33);	
				keybuffer_push(keybuffer, 0x7e);	
			}
			break;
	*/	default:
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


static void wait_ready(void) {
	while (i8042_status_read() & i8042_INPUT_FULL)
		;
}

/** Register uspace irq handler
 * @return 
 */
int kbd_arch_init(void)
{
	int rc1, i;
	int mouseenabled = 0;

	iospace_enable(task_get_id(),(void *)i8042_DATA, 5);
	/* Disable kbd, enable mouse */
	i8042_command_write(i8042_CMD_KBD);
	wait_ready();
	i8042_command_write(i8042_CMD_KBD);
	wait_ready();
	i8042_data_write(i8042_KBD_DISABLE);
	wait_ready();

	/* Flush all current IO */
	while (i8042_status_read() & i8042_OUTPUT_FULL)
		i8042_data_read();
	/* Initialize mouse */
	i8042_command_write(i8042_CMD_MOUSE);
	wait_ready();
	i8042_data_write(MOUSE_OUT_INIT);
	wait_ready();
	
	int mouseanswer = 0;
	for (i=0;i < 1000; i++) {
		int status = i8042_status_read();
		if (status & i8042_OUTPUT_FULL) {
			int data = i8042_data_read();
			if (status & i8042_MOUSE_DATA) {
				mouseanswer = data;
				break;
			}
		}
		usleep(1000);
	}
	if (mouseanswer == MOUSE_ACK) {
		/* enable mouse */
		mouseenabled = 1;

		ipc_register_irq(MOUSE_IRQ, &i8042_kbd);
	}
	/* Enable kbd */
	ipc_register_irq(KBD_IRQ, &i8042_kbd);

	int newcontrol = i8042_KBD_IE | i8042_KBD_TRANSLATE;
	if (mouseenabled)
		newcontrol |= i8042_MOUSE_IE;
	
	i8042_command_write(i8042_CMD_KBD);
	wait_ready();
	i8042_data_write(newcontrol);
	wait_ready();
	
	return 0;
}

/** Process keyboard & mouse events */
int kbd_arch_process(keybuffer_t *keybuffer, ipc_call_t *call)
{
	int status = IPC_GET_ARG1(*call);

	if ((status & i8042_MOUSE_DATA)) {
		;
	} else {
		int scan_code = IPC_GET_ARG2(*call);
		
		if (scan_code != IGNORE_CODE) {
			if (scan_code & KEY_RELEASE)
				key_released(keybuffer, scan_code ^ KEY_RELEASE);
			else
				key_pressed(keybuffer, scan_code);
		}
	}
	return 	1;
}

/**
 * @}
 */ 

