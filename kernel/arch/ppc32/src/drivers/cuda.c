/*
 * Copyright (c) 2006 Martin Decky
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

/** @addtogroup ppc32
 * @{
 */
/** @file
 */

#include <arch/drivers/cuda.h>
#include <arch/asm.h>
#include <console/console.h>
#include <console/chardev.h>
#include <arch/drivers/pic.h>
#include <sysinfo/sysinfo.h>
#include <interrupt.h>
#include <stdarg.h>
#include <ddi/device.h>
#include <string.h>

#define CUDA_IRQ 10

#define PACKET_ADB   0x00
#define PACKET_CUDA  0x01

#define CUDA_POWERDOWN  0x0a
#define CUDA_RESET      0x11

#define RS   0x200
#define B    (0 * RS)
#define A    (1 * RS)
#define SR   (10 * RS)
#define ACR  (11 * RS)

#define SR_OUT  0x10
#define TACK    0x10
#define TIP     0x20

#define SCANCODES  128

static volatile uint8_t *cuda = NULL;
static irq_t cuda_irq;                 /**< Cuda's IRQ. */

static wchar_t lchars[SCANCODES] = {
	'a', 's', 'd', 'f', 'h', 'g', 'z', 'x', 'c', 'v',
	U_SPECIAL,
	'b', 'q', 'w', 'e', 'r', 'y', 't', '1', '2', '3', '4', '6', '5',
	'=', '9', '7', '-', '8', '0', ']', 'o', 'u', '[', 'i', 'p',
	'\n',           /* Enter */
	'l', 'j', '\'', 'k', ';', '\\', ',', '/', 'n', 'm', '.',
	'\t',           /* Tab */
	' ', '`',
	'\b',           /* Backspace */
	U_SPECIAL,
	U_ESCAPE,       /* Escape */
	U_SPECIAL,      /* Ctrl */
	U_SPECIAL,      /* Alt */
	U_SPECIAL,      /* Shift */
	U_SPECIAL,      /* CapsLock */
	U_SPECIAL,      /* Right Alt */
	U_LEFT_ARROW,   /* Left */
	U_RIGHT_ARROW,  /* Right */
	U_DOWN_ARROW,   /* Down */
	U_UP_ARROW,     /* Up */
	U_SPECIAL,
	U_SPECIAL,
	'.',            /* Keypad . */
	U_SPECIAL,
	'*',            /* Keypad * */
	U_SPECIAL,
	'+',            /* Keypad + */
	U_SPECIAL,
	U_SPECIAL,      /* NumLock */
	U_SPECIAL,
	U_SPECIAL,
	U_SPECIAL,
	'/',            /* Keypad / */
	'\n',           /* Keypad Enter */
	U_SPECIAL,
	'-',            /* Keypad - */
	U_SPECIAL,
	U_SPECIAL,
	U_SPECIAL,
	'0',            /* Keypad 0 */
	'1',            /* Keypad 1 */
	'2',            /* Keypad 2 */
	'3',            /* Keypad 3 */
	'4',            /* Keypad 4 */
	'5',            /* Keypad 5 */
	'6',            /* Keypad 6 */
	'7',            /* Keypad 7 */
	U_SPECIAL,
	'8',            /* Keypad 8 */
	'9',            /* Keypad 9 */
	U_SPECIAL,
	U_SPECIAL,
	U_SPECIAL,
	U_SPECIAL,      /* F5 */
	U_SPECIAL,      /* F6 */
	U_SPECIAL,      /* F7 */
	U_SPECIAL,      /* F3 */
	U_SPECIAL,      /* F8 */
	U_SPECIAL,      /* F9 */
	U_SPECIAL,
	U_SPECIAL,      /* F11 */
	U_SPECIAL,
	U_SPECIAL,      /* F13 */
	U_SPECIAL,
	U_SPECIAL,      /* ScrollLock */
	U_SPECIAL,
	U_SPECIAL,      /* F10 */
	U_SPECIAL,
	U_SPECIAL,      /* F12 */
	U_SPECIAL,
	U_SPECIAL,      /* Pause */
	U_SPECIAL,      /* Insert */
	U_HOME_ARROW,   /* Home */
	U_PAGE_UP,      /* Page Up */
	U_DELETE,       /* Delete */
	U_SPECIAL,      /* F4 */
	U_END_ARROW,    /* End */
	U_SPECIAL,      /* F2 */
	U_PAGE_DOWN,    /* Page Down */
	U_SPECIAL       /* F1 */
};

static void receive_packet(uint8_t *kind, index_t count, uint8_t data[])
{
	cuda[B] = cuda[B] & ~TIP;
	*kind = cuda[SR];
	
	index_t i;
	for (i = 0; i < count; i++)
		data[i] = cuda[SR];
	
	cuda[B] = cuda[B] | TIP;
}

static indev_t kbrd;
static indev_operations_t ops = {
	.poll = NULL
};

int cuda_get_scancode(void)
{
	if (cuda) {
		uint8_t kind;
		uint8_t data[4];
		
		receive_packet(&kind, 4, data);
		
		if ((kind == PACKET_ADB) && (data[0] == 0x40) && (data[1] == 0x2c))
			return data[2];
	}
	
	return -1;
}

static void cuda_irq_handler(irq_t *irq)
{
	int scan_code = cuda_get_scancode();
	
	if (scan_code != -1) {
		uint8_t scancode = (uint8_t) scan_code;
		if ((scancode & 0x80) != 0x80)
			indev_push_character(&kbrd, lchars[scancode & 0x7f]);
	}
}

static irq_ownership_t cuda_claim(irq_t *irq)
{
	return IRQ_ACCEPT;
}

void cuda_init(uintptr_t base, size_t size)
{
	cuda = (uint8_t *) hw_map(base, size);
	
	indev_initialize("cuda_kbd", &kbrd, &ops);
	stdin = &kbrd;
	
	irq_initialize(&cuda_irq);
	cuda_irq.devno = device_assign_devno();
	cuda_irq.inr = CUDA_IRQ;
	cuda_irq.claim = cuda_claim;
	cuda_irq.handler = cuda_irq_handler;
	irq_register(&cuda_irq);
	
	pic_enable_interrupt(CUDA_IRQ);
	
	sysinfo_set_item_val("kbd", NULL, true);
	sysinfo_set_item_val("kbd.inr", NULL, CUDA_IRQ);
	sysinfo_set_item_val("kbd.address.virtual", NULL, base);
}

static void send_packet(const uint8_t kind, count_t count, ...)
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
	asm volatile (
		"b 0\n"
	);
}

void arch_reboot(void) {
	if (cuda)
		send_packet(PACKET_CUDA, 1, CUDA_RESET);
	
	asm volatile (
		"b 0\n"
	);
}

/** @}
 */
