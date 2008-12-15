/*
 * Copyright (C) 2006 Martin Decky
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

/** @addtogroup kbdppc32 ppc32
 * @brief	HelenOS ppc32 arch dependent parts of uspace keyboard handler.
 * @ingroup  kbd
 * @{
 */ 
/** @file
 */

#include <arch/kbd.h>
#include <ipc/ipc.h>
#include <sysinfo.h>
#include <kbd.h>
#include <keys.h>

irq_cmd_t cuda_cmds[1] = {
	{ CMD_PPC32_GETCHAR, 0, 0, 2 }
};

irq_code_t cuda_kbd = {
	1,
	cuda_cmds
};


#define SPECIAL		255
#define FUNCTION_KEYS 0x100


static int lchars[0x80] = {
	'a',
	's',
	'd',
	'f',
	'h',
	'g',
	'z',
	'x',
	'c',
	'v', 
	SPECIAL,
	'b',
	'q',
	'w',
	'e',
	'r',
	'y',
	't',
	'1',
	'2',
	'3',
	'4',
	'6',
	'5',
	'=',
	'9',
	'7',
	'-',
	'8',
	'0',
	']',
	'o',
	'u',
	'[',
	'i', 
	'p',
	'\n',                 /* Enter */
	'l',
	'j',
	'\'',
	'k',
	';',
	'\\',
	',',
	'/',
	'n',
	'm',
	'.',
	'\t',                 /* Tab */
	' ',
	'`',
	'\b',                 /* Backspace */
	SPECIAL,
	SPECIAL,              /* Escape */
	SPECIAL,              /* Ctrl */
	SPECIAL,              /* Alt */
	SPECIAL,              /* Shift */
	SPECIAL,              /* Caps-Lock */
	SPECIAL,              /* RAlt */
	SPECIAL,              /* Left */
	SPECIAL,              /* Right */
	SPECIAL,              /* Down */
	SPECIAL,              /* Up */
	SPECIAL, 
	SPECIAL,
	'.',                  /* Keypad . */
	SPECIAL, 
	'*',                  /* Keypad * */
	SPECIAL,
	'+',                  /* Keypad + */
	SPECIAL,
	SPECIAL,              /* NumLock */
	SPECIAL,
	SPECIAL,
	SPECIAL,
	'/',                  /* Keypad / */
	'\n',                 /* Keypad Enter */
	SPECIAL,
	'-',                  /* Keypad - */
	SPECIAL,
	SPECIAL,
	SPECIAL,
	'0',                  /* Keypad 0 */
	'1',                  /* Keypad 1 */
	'2',                  /* Keypad 2 */
	'3',                  /* Keypad 3 */
	'4',                  /* Keypad 4 */
	'5',                  /* Keypad 5 */
	'6',                  /* Keypad 6 */
	'7',                  /* Keypad 7 */
	SPECIAL,
	'8',                  /* Keypad 8 */
	'9',                  /* Keypad 9 */
	SPECIAL,
	SPECIAL,
	SPECIAL,
	(FUNCTION_KEYS | 5),  /* F5 */
	(FUNCTION_KEYS | 6),  /* F6 */
	(FUNCTION_KEYS | 7),  /* F7 */
	(FUNCTION_KEYS | 3),  /* F3 */
	(FUNCTION_KEYS | 8),  /* F8 */
	(FUNCTION_KEYS | 9),  /* F9 */
	SPECIAL,
	(FUNCTION_KEYS | 11), /* F11 */
	SPECIAL,
	(FUNCTION_KEYS | 13), /* F13 */
	SPECIAL,
	SPECIAL,              /* ScrollLock */
	SPECIAL,
	(FUNCTION_KEYS | 10), /* F10 */
	SPECIAL,
	(FUNCTION_KEYS | 12), /* F12 */
	SPECIAL,
	SPECIAL,              /* Pause */
	SPECIAL,              /* Insert */
	SPECIAL,              /* Home */
	SPECIAL,              /* PageUp */
	SPECIAL,              /* Delete */
	(FUNCTION_KEYS | 4),  /* F4 */
	SPECIAL,              /* End */
	(FUNCTION_KEYS | 2),  /* F2 */
	SPECIAL,              /* PageDown */
	(FUNCTION_KEYS | 1)   /* F1 */
};


int kbd_arch_init(void)
{
	return ipc_register_irq(sysinfo_value("cuda.irq"), &cuda_kbd);
}


int kbd_arch_process(keybuffer_t *keybuffer, ipc_call_t *call) 
{
	int param = IPC_GET_ARG2(*call);

	if (param != -1) {
		uint8_t scancode = (uint8_t) param;
	
		if ((scancode & 0x80) != 0x80) {
			int key = lchars[scancode & 0x7f];
			
			if (key != SPECIAL)
				keybuffer_push(keybuffer, key);
		}
	}
	
	return 1;
}

/** @}
 */

