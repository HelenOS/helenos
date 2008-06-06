/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup ia32	
 * @{
 */
/**
 * @file
 * @brief EGA driver.
 */

#include <arch/drivers/ega.h>
#include <putchar.h>
#include <mm/page.h>
#include <mm/as.h>
#include <arch/mm/page.h>
#include <synch/spinlock.h>
#include <arch/types.h>
#include <arch/asm.h>
#include <memstr.h>
#include <console/chardev.h>
#include <console/console.h>
#include <sysinfo/sysinfo.h>
#include <ddi/ddi.h>

/*
 * The EGA driver.
 * Simple and short. Function for displaying characters and "scrolling".
 */

static parea_t ega_parea;	/**< Physical memory area for EGA video RAM. */

SPINLOCK_INITIALIZE(egalock);
static uint32_t ega_cursor;
static uint8_t *videoram;

static void ega_putchar(chardev_t *d, const char ch);

chardev_t ega_console;
static chardev_operations_t ega_ops = {
	.write = ega_putchar
};

static void ega_move_cursor(void);

void ega_init(void)
{
	uint8_t hi, lo;
	
	videoram = (uint8_t *) hw_map(VIDEORAM, SCREEN * 2);
	outb(0x3d4, 0xe);
	hi = inb(0x3d5);
	outb(0x3d4, 0xf);
	lo = inb(0x3d5);
	ega_cursor = (hi << 8) | lo;

	chardev_initialize("ega_out", &ega_console, &ega_ops);
	stdout = &ega_console;
	
	ega_parea.pbase = VIDEORAM;
	ega_parea.vbase = (uintptr_t) videoram;
	ega_parea.frames = 1;
	ega_parea.cacheable = false;
	ddi_parea_register(&ega_parea);

	sysinfo_set_item_val("fb", NULL, true);
	sysinfo_set_item_val("fb.kind", NULL, 2);
	sysinfo_set_item_val("fb.width", NULL, ROW);
	sysinfo_set_item_val("fb.height", NULL, ROWS);
	sysinfo_set_item_val("fb.address.physical", NULL, VIDEORAM);
	sysinfo_set_item_val("fb.address.color", NULL, PAGE_COLOR((uintptr_t)
		videoram));
	
#ifndef CONFIG_FB
	putchar('\n');
#endif	
}

static void ega_display_char(char ch)
{
	videoram[ega_cursor * 2] = ch;
}

/*
 * This function takes care of scrolling.
 */
static void ega_check_cursor(void)
{
	if (ega_cursor < SCREEN)
		return;

	memcpy((void *) videoram, (void *) (videoram + ROW * 2), (SCREEN - ROW) * 2);
	memsetw(videoram + (SCREEN - ROW) * 2, ROW, 0x0720);
	ega_cursor = ega_cursor - ROW;
}

void ega_putchar(chardev_t *d __attribute__((unused)), const char ch)
{
	ipl_t ipl;

	ipl = interrupts_disable();
	spinlock_lock(&egalock);

	switch (ch) {
	case '\n':
		ega_cursor = (ega_cursor + ROW) - ega_cursor % ROW;
		break;
	case '\t':
		ega_cursor = (ega_cursor + 8) - ega_cursor % 8;
		break; 
	case '\b':
		if (ega_cursor % ROW)
			ega_cursor--;
		break;
	default:
		ega_display_char(ch);
		ega_cursor++;
		break;
	}
	ega_check_cursor();
	ega_move_cursor();

	spinlock_unlock(&egalock);
	interrupts_restore(ipl);
}

void ega_move_cursor(void)
{
	outb(0x3d4, 0xe);
	outb(0x3d5, (uint8_t) ((ega_cursor >> 8) & 0xff));
	outb(0x3d4, 0xf);
	outb(0x3d5, (uint8_t) (ega_cursor & 0xff));	
}

/** @}
 */
