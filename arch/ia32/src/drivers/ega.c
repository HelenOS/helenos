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

#include <arch/ega.h>
#include <putchar.h>
#include <mm/page.h>
#include <synch/spinlock.h>
#include <arch/types.h>
#include <arch/asm.h>

/*
 * The EGA driver.
 * Simple and short. Function for displaying characters and "scrolling".
 */

static spinlock_t egalock;
static __u32 ega_cursor;

void ega_move_cursor(void);

void ega_init(void)
{
	__u8 hi, lo;

	map_page_to_frame(VIDEORAM, VIDEORAM, PAGE_NOT_CACHEABLE, 0);
	outb(0x3d4,0xe);
	hi = inb(0x3d5);
	outb(0x3d4,0xf);
	lo = inb(0x3d5);
	ega_cursor = (hi<<8)|lo;
	ega_putchar('\n');
}

void ega_display_char(char ch)
{
	__u8 *vram = (__u8 *) VIDEORAM;
	
	vram[ega_cursor*2] = ch;
}

/*
 * This function takes care of scrolling.
 */
void ega_check_cursor(void)
{
	if (ega_cursor < SCREEN)
	    return;

	memcopy(VIDEORAM + ROW*2, VIDEORAM, (SCREEN - ROW)*2);
	memsetw(VIDEORAM + (SCREEN - ROW)*2, ROW, 0x0720);
	ega_cursor = ega_cursor - ROW;
}

void ega_putchar(const char ch)
{
	pri_t pri;

	pri = cpu_priority_high();
	spinlock_lock(&egalock);

	switch (ch) {
	    case '\n':
		    ega_cursor = (ega_cursor + ROW) - ega_cursor % ROW;
		    break;
	    case '\t':
		    ega_cursor = (ega_cursor + 8) - ega_cursor % 8;
		    break; 
	    default:
		    ega_display_char(ch);
		    ega_cursor++;
		    break;
        }
	ega_check_cursor();
	ega_move_cursor();
        
	spinlock_unlock(&egalock);
	cpu_priority_restore(pri);
}

void ega_move_cursor(void)
{
	outb(0x3d4,0xe);
	outb(0x3d5,(ega_cursor>>8)&0xff);
	outb(0x3d4,0xf);
	outb(0x3d5,ega_cursor&0xff);	
}

void putchar(const char ch)
{
	ega_putchar(ch);
}
