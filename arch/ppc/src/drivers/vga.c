/*
 * Copyright (C) 2005 Martin Decky
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

#include <arch/vga.h>
#include <putchar.h>
#include <mm/page.h>
#include <synch/spinlock.h>
#include <arch/types.h>
#include <arch/asm.h>

static spinlock_t vgalock;
static __u32 vga_cursor;

void vga_move_cursor(void);

void vga_init(void)
{
}

void vga_display_char(char ch)
{
//	__u8 *vram = (__u8 *) PA2KA(VIDEORAM);
	
//	vram[vga_cursor*2] = ch;
}

/*
 * This function takes care of scrolling.
 */
void vga_check_cursor(void)
{
}

void vga_putchar(const char ch)
{
	pri_t pri;

	pri = cpu_priority_high();
	spinlock_lock(&vgalock);

	switch (ch) {
	    case '\n':
		vga_cursor = (vga_cursor + ROW) - vga_cursor % ROW;
		break;
	    case '\t':
		vga_cursor = (vga_cursor + 8) - vga_cursor % 8;
		break; 
	    default:
		vga_display_char(ch);
		vga_cursor++;
		break;
        }
	vga_check_cursor();
	vga_move_cursor();

	spinlock_unlock(&vgalock);
	cpu_priority_restore(pri);
}

void vga_move_cursor(void)
{
}

void putchar(const char ch)
{
	vga_putchar(ch);
}
