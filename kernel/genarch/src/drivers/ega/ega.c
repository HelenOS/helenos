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

/** @addtogroup kernel_genarch
 * @{
 */
/**
 * @file
 * @brief EGA driver.
 */

#include <debug.h>
#include <genarch/drivers/ega/ega.h>
#include <mm/km.h>
#include <mm/as.h>
#include <stdlib.h>
#include <typedefs.h>
#include <arch/asm.h>
#include <memw.h>
#include <str.h>
#include <console/chardev.h>
#include <console/console.h>
#include <sysinfo/sysinfo.h>
#include <ddi/ddi.h>

/*
 * The EGA driver.
 * Simple and short. Function for displaying characters and "scrolling".
 */

#define SPACE  0x20
#define STYLE  0x1e
#define INVAL  0x17

#define EMPTY_CHAR  ((STYLE << 8) | SPACE)

typedef struct {
	IRQ_SPINLOCK_DECLARE(lock);

	parea_t parea;

	uint32_t cursor;
	uint8_t *addr;
	uint8_t *backbuf;
	ioport8_t *base;
} ega_instance_t;

static void ega_putuchar(outdev_t *, char32_t);
static void ega_redraw(outdev_t *);

static outdev_operations_t egadev_ops = {
	.write = ega_putuchar,
	.redraw = ega_redraw,
	.scroll_up = NULL,
	.scroll_down = NULL
};

static uint16_t ega_oem_glyph(const char32_t ch)
{
	if (ch <= 0x007f)
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
static void ega_check_cursor(ega_instance_t *instance)
{
	if (instance->cursor < EGA_SCREEN)
		return;

	memmove((void *) instance->backbuf,
	    (void *) (instance->backbuf + EGA_COLS * 2),
	    (EGA_SCREEN - EGA_COLS) * 2);
	memsetw(instance->backbuf + (EGA_SCREEN - EGA_COLS) * 2,
	    EGA_COLS, EMPTY_CHAR);

	if ((!instance->parea.mapped) || (console_override)) {
		memmove((void *) instance->addr,
		    (void *) (instance->addr + EGA_COLS * 2),
		    (EGA_SCREEN - EGA_COLS) * 2);
		memsetw(instance->addr + (EGA_SCREEN - EGA_COLS) * 2,
		    EGA_COLS, EMPTY_CHAR);
	}

	instance->cursor = instance->cursor - EGA_COLS;
}

static void ega_show_cursor(ega_instance_t *instance)
{
	if ((!instance->parea.mapped) || (console_override)) {
		pio_write_8(instance->base + EGA_INDEX_REG, 0x0a);
		uint8_t stat = pio_read_8(instance->base + EGA_DATA_REG);
		pio_write_8(instance->base + EGA_INDEX_REG, 0x0a);
		pio_write_8(instance->base + EGA_DATA_REG, stat & (~(1 << 5)));
	}
}

static void ega_move_cursor(ega_instance_t *instance)
{
	if ((!instance->parea.mapped) || (console_override)) {
		pio_write_8(instance->base + EGA_INDEX_REG, 0x0e);
		pio_write_8(instance->base + EGA_DATA_REG,
		    (uint8_t) ((instance->cursor >> 8) & 0xff));
		pio_write_8(instance->base + EGA_INDEX_REG, 0x0f);
		pio_write_8(instance->base + EGA_DATA_REG,
		    (uint8_t) (instance->cursor & 0xff));
	}
}

static void ega_sync_cursor(ega_instance_t *instance)
{
	if ((!instance->parea.mapped) || (console_override)) {
		pio_write_8(instance->base + EGA_INDEX_REG, 0x0e);
		uint8_t hi = pio_read_8(instance->base + EGA_DATA_REG);
		pio_write_8(instance->base + EGA_INDEX_REG, 0x0f);
		uint8_t lo = pio_read_8(instance->base + EGA_DATA_REG);

		instance->cursor = (hi << 8) | lo;
	} else
		instance->cursor = 0;

	if (instance->cursor >= EGA_SCREEN)
		instance->cursor = 0;

	if ((instance->cursor % EGA_COLS) != 0)
		instance->cursor =
		    (instance->cursor + EGA_COLS) - instance->cursor % EGA_COLS;

	memsetw(instance->backbuf + instance->cursor * 2,
	    EGA_SCREEN - instance->cursor, EMPTY_CHAR);

	if ((!instance->parea.mapped) || (console_override))
		memsetw(instance->addr + instance->cursor * 2,
		    EGA_SCREEN - instance->cursor, EMPTY_CHAR);

	ega_check_cursor(instance);
	ega_move_cursor(instance);
	ega_show_cursor(instance);
}

static void ega_display_wchar(ega_instance_t *instance, char32_t ch)
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

	instance->backbuf[instance->cursor * 2] = glyph;
	instance->backbuf[instance->cursor * 2 + 1] = style;

	if ((!instance->parea.mapped) || (console_override)) {
		instance->addr[instance->cursor * 2] = glyph;
		instance->addr[instance->cursor * 2 + 1] = style;
	}
}

static void ega_putuchar(outdev_t *dev, char32_t ch)
{
	ega_instance_t *instance = (ega_instance_t *) dev->data;

	irq_spinlock_lock(&instance->lock, true);

	switch (ch) {
	case '\n':
		instance->cursor = (instance->cursor + EGA_COLS) -
		    instance->cursor % EGA_COLS;
		break;
	case '\t':
		instance->cursor = (instance->cursor + 8) -
		    instance->cursor % 8;
		break;
	case '\b':
		if (instance->cursor % EGA_COLS)
			instance->cursor--;
		break;
	default:
		ega_display_wchar(instance, ch);
		instance->cursor++;
		break;
	}
	ega_check_cursor(instance);
	ega_move_cursor(instance);

	irq_spinlock_unlock(&instance->lock, true);
}

static void ega_redraw_internal(ega_instance_t *instance)
{
	memcpy(instance->addr, instance->backbuf, EGA_VRAM_SIZE);
	ega_move_cursor(instance);
	ega_show_cursor(instance);
}

static void ega_redraw(outdev_t *dev)
{
	ega_instance_t *instance = (ega_instance_t *) dev->data;

	irq_spinlock_lock(&instance->lock, true);
	ega_redraw_internal(instance);
	irq_spinlock_unlock(&instance->lock, true);
}

/** EGA was mapped or unmapped.
 *
 * @param arg EGA instance
 */
static void ega_mapped_changed(void *arg)
{
	ega_instance_t *instance = (ega_instance_t *) arg;

	if (!instance->parea.mapped) {
		irq_spinlock_lock(&instance->lock, true);
		ega_redraw_internal(instance);
		irq_spinlock_unlock(&instance->lock, true);
	}
}

outdev_t *ega_init(ioport8_t *base, uintptr_t addr)
{
	outdev_t *egadev = malloc(sizeof(outdev_t));
	if (!egadev)
		return NULL;

	ega_instance_t *instance = malloc(sizeof(ega_instance_t));
	if (!instance) {
		free(egadev);
		return NULL;
	}

	outdev_initialize("egadev", egadev, &egadev_ops);
	egadev->data = instance;

	irq_spinlock_initialize(&instance->lock, "*ega.instance.lock");

	instance->base = base;
	instance->addr = (uint8_t *) km_map(addr, EGA_VRAM_SIZE,
	    KM_NATURAL_ALIGNMENT, PAGE_WRITE | PAGE_NOT_CACHEABLE);
	if (!instance->addr) {
		LOG("Unable to EGA video memory.");
		free(instance);
		free(egadev);
		return NULL;
	}

	instance->backbuf = (uint8_t *) malloc(EGA_VRAM_SIZE);
	if (!instance->backbuf) {
		LOG("Unable to allocate backbuffer.");
		free(instance);
		free(egadev);
		return NULL;
	}

	ddi_parea_init(&instance->parea);
	instance->parea.pbase = addr;
	instance->parea.frames = SIZE2FRAMES(EGA_VRAM_SIZE);
	instance->parea.unpriv = false;
	instance->parea.mapped = false;
	instance->parea.mapped_changed = ega_mapped_changed;
	instance->parea.arg = (void *) instance;
	ddi_parea_register(&instance->parea);

	/* Synchronize the back buffer and cursor position. */
	memcpy(instance->backbuf, instance->addr, EGA_VRAM_SIZE);
	ega_sync_cursor(instance);

	if (!fb_exported) {
		/*
		 * We export the kernel framebuffer for uspace usage.
		 * This is used in the case the uspace framebuffer
		 * driver is not self-sufficient.
		 */
		sysinfo_set_item_val("fb", NULL, true);
		sysinfo_set_item_val("fb.kind", NULL, 2);
		sysinfo_set_item_val("fb.width", NULL, EGA_COLS);
		sysinfo_set_item_val("fb.height", NULL, EGA_ROWS);
		sysinfo_set_item_val("fb.blinking", NULL, true);
		sysinfo_set_item_val("fb.address.physical", NULL, addr);

		fb_exported = true;
	}

	return egadev;
}

/** @}
 */
