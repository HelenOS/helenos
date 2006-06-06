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

#include <arch/kbd.h>
#include <ipc/ipc.h>
#include <sysinfo.h>

irq_cmd_t cuda_cmds[1] = {
	{ CMD_PPC32_GETCHAR, 0, 0 }
};

irq_code_t cuda_kbd = {
	1,
	cuda_cmds
};


static char lchars[0x80] = {
	'a',  's',  'd',  'f',  'h',  'g',  'z',  'x',  'c',  'v',    0,  'b',  'q',  'w',  'e',  'r',
	'y',  't',  '1',  '2',  '3',  '4',  '6',  '5',  '=',  '9',  '7',  '-',  '8',  '0',  ']',  'o',
	'u',  '[',  'i',  'p',   13,  'l',  'j', '\'',  'k',  ';', '\\',  ',',  '/',  'n',  'm',  '.',
	  9,   32,  '`',    8,    0,   27,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	  0,  '.',    0,  '*',    0,  '+',    0,    0,    0,    0,    0,  '/',   13,    0,  '-',    0,
	  0,    0,  '0',  '1',  '2',  '3',  '4',  '5',  '6',  '7',    0,  '8',  '9',    0,    0,    0,
	  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
	  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0
};


int kbd_arch_init(void)
{
	return (!ipc_register_irq(sysinfo_value("cuda.irq"), &cuda_kbd));
}


int kbd_arch_process(keybuffer_t *keybuffer, int scan_code) 
{
	uint8_t scancode = (uint8_t) scan_code;
	
	if ((scancode != 0) && ((scancode & 0x80) != 0x80))
		keybuffer_push(keybuffer, lchars[scancode & 0x7f]);
	
	return 1;
}
