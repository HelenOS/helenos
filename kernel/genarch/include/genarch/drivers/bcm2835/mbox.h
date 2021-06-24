/*
 * Copyright (c) 2013 Beniamino Galvani
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
 * @brief BCM2835 mailbox communication routines
 */

#ifndef _BCM2835_MBOX_H_
#define _BCM2835_MBOX_H_

#include <genarch/fb/fb.h>
#include <arch/mm/page.h>
#include <align.h>

#define BCM2835_MBOX0_ADDR	0x2000B880

enum {
	MBOX_CHAN_PM		= 0,
	MBOX_CHAN_FB		= 1,
	MBOX_CHAN_UART		= 2,
	MBOX_CHAN_VCHIQ		= 3,
	MBOX_CHAN_LED		= 4,
	MBOX_CHAN_BTN		= 5,
	MBOX_CHAN_TS		= 6,
	MBOX_CHAN_PROP_A2V	= 8,
	MBOX_CHAN_PROP_V2A	= 9
};

enum {
	TAG_GET_FW_REV		= 0x00000001,
	TAG_GET_BOARD_MODEL	= 0x00010001,
	TAG_GET_BOARD_REV	= 0x00010002,
	TAG_GET_BOARD_MAC	= 0x00010003,
	TAG_GET_BOARD_SERIAL	= 0x00010004,
	TAG_GET_ARM_MEMORY	= 0x00010005,
	TAG_GET_VC_MEMORY	= 0x00010006,
	TAG_GET_CLOCKS		= 0x00010007,
	TAG_GET_CMD_LINE	= 0x00050001,
};

enum {
	MBOX_TAG_GET_PHYS_W_H	= 0x00040003,
	MBOX_TAG_SET_PHYS_W_H	= 0x00048003,
	MBOX_TAG_GET_VIRT_W_H	= 0x00040004,
	MBOX_TAG_SET_VIRT_W_G	= 0x00048004
};

enum {
	MBOX_PROP_CODE_REQ	= 0x00000000,
	MBOX_PROP_CODE_RESP_OK	= 0x80000000,
	MBOX_PROP_CODE_RESP_ERR	= 0x80000001
};

#define MBOX_STATUS_FULL	(1 << 31)
#define MBOX_STATUS_EMPTY	(1 << 30)

#define MBOX_COMPOSE(chan, value) (((chan) & 0xf) | ((value) & ~0xf))
#define MBOX_MSG_CHAN(msg)	((msg) & 0xf)
#define MBOX_MSG_VALUE(msg)	((msg) & ~0xf)

#define KA2VCA(addr)		(KA2PA(addr) + 0x40000000)

#define MBOX_ADDR_ALIGN		16

#define MBOX_BUFF_ALLOC(name, type)					\
	char tmp_ ## name[sizeof(type) + MBOX_ADDR_ALIGN] = { 0 };      \
	type *name = (type *)ALIGN_UP((uintptr_t)tmp_ ## name, MBOX_ADDR_ALIGN);

typedef struct {
	ioport32_t read;
	ioport32_t unused[3];
	ioport32_t peek;
	ioport32_t sender;
	ioport32_t status;
	ioport32_t config;
	ioport32_t write;
} bcm2835_mbox_t;

typedef struct {
	ioport32_t size;
	ioport32_t code;
} mbox_prop_buf_hdr_t;

typedef struct {
	ioport32_t tag_id;
	ioport32_t buf_size;
	ioport32_t val_len;
} mbox_tag_hdr_t;

typedef struct {
	ioport32_t base;
	ioport32_t size;
} mbox_tag_getmem_resp_t;

typedef struct {
	mbox_prop_buf_hdr_t	buf_hdr;
	mbox_tag_hdr_t		tag_hdr;
	mbox_tag_getmem_resp_t	data;
	uint32_t		zero;
} mbox_getmem_buf_t;

typedef struct {
	mbox_prop_buf_hdr_t	buf_hdr;
	mbox_tag_hdr_t		tag_hdr;
	struct {
		uint32_t	width;
		uint32_t	height;
	} body;
	uint32_t zero;
} mbox_getfbsize_buf_t;

typedef struct {
	ioport32_t width;
	ioport32_t height;
	ioport32_t virt_width;
	ioport32_t virt_height;
	ioport32_t pitch;
	ioport32_t bpp;
	ioport32_t x_offset;
	ioport32_t y_offset;
	ioport32_t addr;
	ioport32_t size;
} bcm2835_fb_desc_t;

extern bool bcm2835_prop_get_memory(uint32_t *base, uint32_t *size);
extern bool bcm2835_fb_init(fb_properties_t *prop, uint32_t width, uint32_t heigth);
extern bool bcm2835_mbox_get_fb_size(uint32_t *h, uint32_t *w);

#endif

/**
 * @}
 */
