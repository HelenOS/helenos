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

/** @addtogroup genarch_drivers
 * @{
 */
/**
 * @file
 * @brief EGA driver.
 */

#include <genarch/drivers/ega/ega.h>
#include <putchar.h>
#include <mm/page.h>
#include <mm/as.h>
#include <mm/slab.h>
#include <arch/mm/page.h>
#include <synch/spinlock.h>
#include <arch/types.h>
#include <arch/asm.h>
#include <memstr.h>
#include <string.h>
#include <console/chardev.h>
#include <console/console.h>
#include <sysinfo/sysinfo.h>
#include <ddi/ddi.h>

/*
 * The EGA driver.
 * Simple and short. Function for displaying characters and "scrolling".
 */

SPINLOCK_INITIALIZE(egalock);
static uint32_t ega_cursor;
static uint8_t *videoram;
static uint8_t *backbuf;
static ioport8_t *ega_base;

#define SPACE  0x20
#define STYLE  0x1e
#define INVAL  0x17

#define EMPTY_CHAR  ((STYLE << 8) | SPACE)

static uint16_t ega_oem_glyph(const wchar_t ch)
{
	if ((ch >= 0x0000) && (ch <= 0x007f))
		return ch;
	
	if (ch == 0x00a0)
		return 255;
	
	if (ch == 0x00a1)
		return 173;
	
	if ((ch >= 0x00a2) && (ch <= 0x00a3))
		return (ch - 7);
	
	if (ch == 0x00a5)
		return 157;
	
	if (ch == 0x00aa)
		return 166;
	
	if (ch == 0x00ab)
		return 174;
	
	if (ch == 0x00ac)
		return 170;
	
	if (ch == 0x00b0)
		return 248;
	
	if (ch == 0x00b1)
		return 241;
	
	if (ch == 0x00b2)
		return 253;
	
	if (ch == 0x00b5)
		return 230;
	
	if (ch == 0x00b7)
		return 250;
	
	if (ch == 0x00ba)
		return 167;
	
	if (ch == 0x00bb)
		return 175;
	
	if (ch == 0x00bc)
		return 172;
	
	if (ch == 0x00bd)
		return 171;
	
	if (ch == 0x00bf)
		return 168;
	
	if ((ch >= 0x00c4) && (ch <= 0x00c5))
		return (ch - 54);
	
	if (ch == 0x00c6)
		return 146;
	
	if (ch == 0x00c7)
		return 128;
	
	if (ch == 0x00c9)
		return 144;
	
	if (ch == 0x00d1)
		return 165;
	
	if (ch == 0x00d6)
		return 153;
	
	if (ch == 0x00dc)
		return 154;
	
	if (ch == 0x00df)
		return 225;
	
	if (ch == 0x00e0)
		return 133;
	
	if (ch == 0x00e1)
		return 160;
	
	if (ch == 0x00e2)
		return 131;
	
	if (ch == 0x00e4)
		return 132;
	
	if (ch == 0x00e5)
		return 134;
	
	if (ch == 0x00e6)
		return 145;
	
	if (ch == 0x00e7)
		return 135;
	
	if (ch == 0x00e8)
		return 138;
	
	if (ch == 0x00e9)
		return 130;
	
	if ((ch >= 0x00ea) && (ch <= 0x00eb))
		return (ch - 98);
	
	if (ch == 0x00ec)
		return 141;
	
	if (ch == 0x00ed)
		return 161;
	
	if (ch == 0x00ee)
		return 140;
	
	if (ch == 0x00ef)
		return 139;
	
	if (ch == 0x00f1)
		return 164;
	
	if (ch == 0x00f2)
		return 149;
	
	if (ch == 0x00f3)
		return 162;
	
	if (ch == 0x00f4)
		return 147;
	
	if (ch == 0x00f6)
		return 148;
	
	if (ch == 0x00f7)
		return 246;
	
	if (ch == 0x00f9)
		return 151;
	
	if (ch == 0x00fa)
		return 163;
	
	if (ch == 0x00fb)
		return 150;
	
	if (ch == 0x00fc)
		return 129;
	
	if (ch == 0x00ff)
		return 152;
	
	if (ch == 0x0192)
		return 159;
	
	if (ch == 0x0393)
		return 226;
	
	if (ch == 0x0398)
		return 233;
	
	if (ch == 0x03a3)
		return 228;
	
	if (ch == 0x03a6)
		return 232;
	
	if (ch == 0x03a9)
		return 234;
	
	if (ch == 0x03b1)
		return 224;
	
	if (ch == 0x03b4)
		return 235;
	
	if (ch == 0x03b5)
		return 238;
	
	if (ch == 0x03c0)
		return 227;
	
	if (ch == 0x03c3)
		return 229;
	
	if (ch == 0x03c4)
		return 231;
	
	if (ch == 0x03c6)
		return 237;
	
	if (ch == 0x207f)
		return 252;
	
	if (ch == 0x20a7)
		return 158;
	
	if (ch == 0x2219)
		return 249;
	
	if (ch == 0x221a)
		return 251;
	
	if (ch == 0x221e)
		return 236;
	
	if (ch == 0x2229)
		return 239;
	
	if (ch == 0x2248)
		return 247;
	
	if (ch == 0x2261)
		return 240;
	
	if (ch == 0x2264)
		return 243;
	
	if (ch == 0x2265)
		return 242;
	
	if (ch == 0x2310)
		return 169;
	
	if ((ch >= 0x2320) && (ch <= 0x2321))
		return (ch - 8748);
	
	if (ch == 0x2500)
		return 196;
	
	if (ch == 0x2502)
		return 179;
	
	if (ch == 0x250c)
		return 218;
	
	if (ch == 0x2510)
		return 191;
	
	if (ch == 0x2514)
		return 192;
	
	if (ch == 0x2518)
		return 217;
	
	if (ch == 0x251c)
		return 195;
	
	if (ch == 0x2524)
		return 180;
	
	if (ch == 0x252c)
		return 194;
	
	if (ch == 0x2534)
		return 193;
	
	if (ch == 0x253c)
		return 197;
	
	if (ch == 0x2550)
		return 205;
	
	if (ch == 0x2551)
		return 186;
	
	if ((ch >= 0x2552) && (ch <= 0x2553))
		return (ch - 9341);
	
	if (ch == 0x2554)
		return 201;
	
	if (ch == 0x2555)
		return 184;
	
	if (ch == 0x2556)
		return 183;
	
	if (ch == 0x2557)
		return 187;
	
	if (ch == 0x2558)
		return 212;
	
	if (ch == 0x2559)
		return 211;
	
	if (ch == 0x255a)
		return 200;
	
	if (ch == 0x255b)
		return 190;
	
	if (ch == 0x255c)
		return 189;
	
	if (ch == 0x255d)
		return 188;
	
	if ((ch >= 0x255e) && (ch <= 0x255f))
		return (ch - 9368);
	
	if (ch == 0x2560)
		return 204;
	
	if ((ch >= 0x2561) && (ch <= 0x2562))
		return (ch - 9388);
	
	if (ch == 0x2563)
		return 185;
	
	if ((ch >= 0x2564) && (ch <= 0x2565))
		return (ch - 9363);
	
	if (ch == 0x2566)
		return 203;
	
	if ((ch >= 0x2567) && (ch <= 0x2568))
		return (ch - 9368);
	
	if (ch == 0x2569)
		return 202;
	
	if (ch == 0x256a)
		return 216;
	
	if (ch == 0x256b)
		return 215;
	
	if (ch == 0x256c)
		return 206;
	
	if (ch == 0x2580)
		return 223;
	
	if (ch == 0x2584)
		return 220;
	
	if (ch == 0x2588)
		return 219;
	
	if (ch == 0x258c)
		return 221;
	
	if (ch == 0x2590)
		return 222;
	
	if ((ch >= 0x2591) && (ch <= 0x2593))
		return (ch - 9441);
	
	return 256;
}

/*
 * This function takes care of scrolling.
 */
static void ega_check_cursor(bool silent)
{
	if (ega_cursor < EGA_SCREEN)
		return;
	
	memmove((void *) backbuf, (void *) (backbuf + EGA_COLS * 2),
	    (EGA_SCREEN - EGA_COLS) * 2);
	memsetw(backbuf + (EGA_SCREEN - EGA_COLS) * 2, EGA_COLS, EMPTY_CHAR);
	
	if (!silent) {
		memmove((void *) videoram, (void *) (videoram + EGA_COLS * 2),
		    (EGA_SCREEN - EGA_COLS) * 2);
		memsetw(videoram + (EGA_SCREEN - EGA_COLS) * 2, EGA_COLS, EMPTY_CHAR);
	}
	
	ega_cursor = ega_cursor - EGA_COLS;
}

static void ega_show_cursor(bool silent)
{
	if (!silent) {
		pio_write_8(ega_base + EGA_INDEX_REG, 0x0a);
		uint8_t stat = pio_read_8(ega_base + EGA_DATA_REG);
		pio_write_8(ega_base + EGA_INDEX_REG, 0x0a);
		pio_write_8(ega_base + EGA_DATA_REG, stat & (~(1 << 5)));
	}
}

static void ega_move_cursor(bool silent)
{
	if (!silent) {
		pio_write_8(ega_base + EGA_INDEX_REG, 0x0e);
		pio_write_8(ega_base + EGA_DATA_REG, (uint8_t) ((ega_cursor >> 8) & 0xff));
		pio_write_8(ega_base + EGA_INDEX_REG, 0x0f);
		pio_write_8(ega_base + EGA_DATA_REG, (uint8_t) (ega_cursor & 0xff));
	}
}

static void ega_sync_cursor(bool silent)
{
	if (!silent) {
		pio_write_8(ega_base + EGA_INDEX_REG, 0x0e);
		uint8_t hi = pio_read_8(ega_base + EGA_DATA_REG);
		pio_write_8(ega_base + EGA_INDEX_REG, 0x0f);
		uint8_t lo = pio_read_8(ega_base + EGA_DATA_REG);
		
		ega_cursor = (hi << 8) | lo;
	} else
		ega_cursor = 0;
	
	if (ega_cursor >= EGA_SCREEN)
		ega_cursor = 0;
	
	if ((ega_cursor % EGA_COLS) != 0)
		ega_cursor = (ega_cursor + EGA_COLS) - ega_cursor % EGA_COLS;
	
	memsetw(backbuf + ega_cursor * 2, EGA_SCREEN - ega_cursor, EMPTY_CHAR);
	
	if (!silent)
		memsetw(videoram + ega_cursor * 2, EGA_SCREEN - ega_cursor, EMPTY_CHAR);
	
	ega_check_cursor(silent);
	ega_move_cursor(silent);
	ega_show_cursor(silent);
}

static void ega_display_char(wchar_t ch, bool silent)
{
	uint16_t index = ega_oem_glyph(ch);
	uint8_t glyph;
	uint8_t style;
	
	if ((index >> 8)) {
		glyph = U_SPECIAL;
		style = INVAL;
	} else {
		glyph = index & 0xff;
		style = STYLE;
	}
	
	backbuf[ega_cursor * 2] = glyph;
	backbuf[ega_cursor * 2 + 1] = style;
	
	if (!silent) {
		videoram[ega_cursor * 2] = glyph;
		videoram[ega_cursor * 2 + 1] = style;
	}
}

static void ega_putchar(outdev_t *dev __attribute__((unused)), wchar_t ch, bool silent)
{
	ipl_t ipl;
	
	ipl = interrupts_disable();
	spinlock_lock(&egalock);
	
	switch (ch) {
	case '\n':
		ega_cursor = (ega_cursor + EGA_COLS) - ega_cursor % EGA_COLS;
		break;
	case '\t':
		ega_cursor = (ega_cursor + 8) - ega_cursor % 8;
		break;
	case '\b':
		if (ega_cursor % EGA_COLS)
			ega_cursor--;
		break;
	default:
		ega_display_char(ch, silent);
		ega_cursor++;
		break;
	}
	ega_check_cursor(silent);
	ega_move_cursor(silent);
	
	spinlock_unlock(&egalock);
	interrupts_restore(ipl);
}

static outdev_t ega_console;
static outdev_operations_t ega_ops = {
	.write = ega_putchar
};

void ega_init(ioport8_t *base, uintptr_t videoram_phys)
{
	/* Initialize the software structure. */
	ega_base = base;
	
	backbuf = (uint8_t *) malloc(EGA_VRAM_SIZE, 0);
	if (!backbuf)
		panic("Unable to allocate backbuffer.");
	
	videoram = (uint8_t *) hw_map(videoram_phys, EGA_VRAM_SIZE);
	
	/* Synchronize the back buffer and cursor position. */
	memcpy(backbuf, videoram, EGA_VRAM_SIZE);
	ega_sync_cursor(silent);
	
	outdev_initialize("ega", &ega_console, &ega_ops);
	stdout_wire(&ega_console);
	
	sysinfo_set_item_val("fb", NULL, true);
	sysinfo_set_item_val("fb.kind", NULL, 2);
	sysinfo_set_item_val("fb.width", NULL, EGA_COLS);
	sysinfo_set_item_val("fb.height", NULL, EGA_ROWS);
	sysinfo_set_item_val("fb.blinking", NULL, true);
	sysinfo_set_item_val("fb.address.physical", NULL, videoram_phys);
}

void ega_redraw(void)
{
	memcpy(videoram, backbuf, EGA_VRAM_SIZE);
	ega_move_cursor(silent);
	ega_show_cursor(silent);
}

/** @}
 */
