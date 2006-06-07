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

#include <arch/drivers/cuda.h>
#include <arch/asm.h>
#include <console/console.h>
#include <console/chardev.h>
#include <arch/drivers/pic.h>
#include <sysinfo/sysinfo.h>
#include <interrupt.h>
#include <stdarg.h>

#define SPECIAL		'?'

#define PACKET_ADB  0x00
#define PACKET_CUDA 0x01

#define CUDA_POWERDOWN 0x0a

#define RS 0x200
#define B (0 * RS)
#define A (1 * RS)
#define SR (10 * RS)
#define ACR (11 * RS)

#define SR_OUT 0x10
#define TACK 0x10
#define TIP 0x20


static volatile __u8 *cuda = NULL;


static char lchars[0x80] = {
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
	'\n',    /* Enter */
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
	'\t',    /* Tab */
	' ',
	'`',
	'\b',    /* Backspace */
	SPECIAL,
	SPECIAL, /* Escape */
	SPECIAL, /* Ctrl */
	SPECIAL, /* Alt */
	SPECIAL, /* Shift */
	SPECIAL, /* Caps-Lock */
	SPECIAL, /* RAlt */
	SPECIAL, /* Left */
	SPECIAL, /* Right */
	SPECIAL, /* Down */
	SPECIAL, /* Up */
	SPECIAL, 
	SPECIAL,
	'.',     /* Keypad . */
	SPECIAL, 
	'*',     /* Keypad * */
	SPECIAL,
	'+',     /* Keypad + */
	SPECIAL,
	SPECIAL, /* NumLock */
	SPECIAL,
	SPECIAL,
	SPECIAL,
	'/',     /* Keypad / */
	'\n',    /* Keypad Enter */
	SPECIAL,
	'-',     /* Keypad - */
	SPECIAL,
	SPECIAL,
	SPECIAL,
	'0',     /* Keypad 0 */
	'1',     /* Keypad 1 */
	'2',     /* Keypad 2 */
	'3',     /* Keypad 3 */
	'4',     /* Keypad 4 */
	'5',     /* Keypad 5 */
	'6',     /* Keypad 6 */
	'7',     /* Keypad 7 */
	SPECIAL,
	'8',     /* Keypad 8 */
	'9',     /* Keypad 9 */
	SPECIAL,
	SPECIAL,
	SPECIAL,
	SPECIAL, /* F5 */
	SPECIAL, /* F6 */
	SPECIAL, /* F7 */
	SPECIAL, /* F3 */
	SPECIAL, /* F8 */
	SPECIAL, /* F9 */
	SPECIAL,
	SPECIAL, /* F11 */
	SPECIAL,
	SPECIAL, /* F13 */
	SPECIAL,
	SPECIAL, /* ScrollLock */
	SPECIAL,
	SPECIAL, /* F10 */
	SPECIAL,
	SPECIAL, /* F12 */
	SPECIAL,
	SPECIAL, /* Pause */
	SPECIAL, /* Insert */
	SPECIAL, /* Home */
	SPECIAL, /* PageUp */
	SPECIAL, /* Delete */
	SPECIAL, /* F4 */
	SPECIAL, /* End */
	SPECIAL, /* F2 */
	SPECIAL, /* PageDown */
	SPECIAL  /* F1 */
};


void send_packet(const __u8 kind, index_t count, ...);


static void receive_packet(__u8 *kind, index_t count, __u8 data[])
{
	cuda[B] = cuda[B] & ~TIP;
	*kind = cuda[SR];
	
	index_t i;
	for (i = 0; i < count; i++)
		data[i] = cuda[SR];
	
	cuda[B] = cuda[B] | TIP;
}


/* Called from getc(). */
static void cuda_resume(chardev_t *d)
{
}


/* Called from getc(). */
static void cuda_suspend(chardev_t *d)
{
}


static char key_read(chardev_t *d)
{
	char ch;
	
	ch = 0;
	return ch;
}


static chardev_t kbrd;
static chardev_operations_t ops = {
	.suspend = cuda_suspend,
	.resume = cuda_resume,
	.read = key_read
};


int cuda_get_scancode(void)
{
	__u8 kind;
	__u8 data[4];
	
	receive_packet(&kind, 4, data);
	
	if ((kind == PACKET_ADB) && (data[0] == 0x40) && (data[1] == 0x2c))
		return data[2];
	
	return -1;
}

static void cuda_irq(int n, istate_t *istate)
{
	int scan_code = cuda_get_scancode();
	
	if (scan_code != -1) {
		__u8 scancode = (__u8) scan_code;
		if ((scancode & 0x80) != 0x80)
			chardev_push_character(&kbrd, lchars[scancode & 0x7f]);
	}
}


void cuda_init(__address base, size_t size)
{
	cuda = (__u8 *) hw_map(base, size);
	
	int_register(CUDA_IRQ, "cuda", cuda_irq);
	pic_enable_interrupt(CUDA_IRQ);
	
	chardev_initialize("cuda_kbd", &kbrd, &ops);
	stdin = &kbrd;
	
	sysinfo_set_item_val("cuda", NULL, true);
	sysinfo_set_item_val("cuda.irq", NULL, CUDA_IRQ);
}


void send_packet(const __u8 kind, index_t count, ...)
{
	index_t i;
	va_list va;
	
	cuda[B] = cuda[B] | TIP;
	cuda[ACR] = cuda[ACR] | SR_OUT;
	cuda[SR] = kind;
	cuda[B] = cuda[B] & ~TIP;
	
	va_start(va, count);
	
	for (i = 0; i < count; i++) {
		cuda[ACR] = cuda[ACR] | SR_OUT;
		cuda[SR] = va_arg(va, int);
		cuda[B] = cuda[B] | TACK;
	}
	
	va_end(va);
	
	cuda[B] = cuda[B] | TIP;
}


void cpu_halt(void) {
#ifdef CONFIG_POWEROFF
	send_packet(PACKET_CUDA, 1, CUDA_POWERDOWN);
#endif
	asm volatile (
		"b 0\n"
	);
}
