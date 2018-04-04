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
/** @addtogroup genarch
 * @{
 */
/**
 * @file
 * @brief BCM2835 mailbox communication routines
 */

#include <mm/km.h>
#include <typedefs.h>
#include <genarch/drivers/bcm2835/mbox.h>

static void mbox_write(bcm2835_mbox_t *mbox, uint8_t chan, uint32_t value)
{
	while (mbox->status & MBOX_STATUS_FULL)
		;
	mbox->write = MBOX_COMPOSE(chan, value);
}

static uint32_t mbox_read(bcm2835_mbox_t *mbox, uint8_t chan)
{
	uint32_t msg;

	do {
		while (mbox->status & MBOX_STATUS_EMPTY)
			;
		msg = mbox->read;
	} while (MBOX_MSG_CHAN(msg) != chan);

	return MBOX_MSG_VALUE(msg);
}

bool bcm2835_prop_get_memory(uint32_t *base, uint32_t *size)
{
	bool ret;
	MBOX_BUFF_ALLOC(req, mbox_getmem_buf_t);

	req->buf_hdr.size = sizeof(mbox_getmem_buf_t);
	req->buf_hdr.code = MBOX_PROP_CODE_REQ;
	req->tag_hdr.tag_id = TAG_GET_ARM_MEMORY;
	req->tag_hdr.buf_size = sizeof(mbox_tag_getmem_resp_t);
	req->tag_hdr.val_len = 0;
	req->zero = 0;

	mbox_write((bcm2835_mbox_t *)BCM2835_MBOX0_ADDR,
	    MBOX_CHAN_PROP_A2V, KA2VCA((uint32_t)req));
	mbox_read((bcm2835_mbox_t *)BCM2835_MBOX0_ADDR,
	    MBOX_CHAN_PROP_A2V);

	if (req->buf_hdr.code == MBOX_PROP_CODE_RESP_OK) {
		*base = req->data.base;
		*size = req->data.size;
		ret = true;
	} else {
		ret = false;
	}

	return ret;
}

bool bcm2835_fb_init(fb_properties_t *prop)
{
	bcm2835_mbox_t *fb_mbox;
	bool ret = false;
	MBOX_BUFF_ALLOC(fb_desc, bcm2835_fb_desc_t);

	fb_mbox = (void *) km_map(BCM2835_MBOX0_ADDR, sizeof(bcm2835_mbox_t),
	    PAGE_NOT_CACHEABLE);

	fb_desc->width = 640;
	fb_desc->height = 480;
	fb_desc->virt_width = fb_desc->width;
	fb_desc->virt_height = fb_desc->height;
	fb_desc->pitch = 0;			/* Set by VC */
	fb_desc->bpp = 16;
	fb_desc->x_offset = 0;
	fb_desc->y_offset = 0;
	fb_desc->addr = 0;			/* Set by VC */
	fb_desc->size = 0;			/* Set by VC */

	mbox_write(fb_mbox, MBOX_CHAN_FB, KA2VCA(fb_desc));

	if (mbox_read(fb_mbox, MBOX_CHAN_FB)) {
		printf("BCM2835 framebuffer initialization failed\n");
		goto out;
	}

	prop->addr = fb_desc->addr;
	prop->offset = 0;
	prop->x = fb_desc->width;
	prop->y = fb_desc->height;
	prop->scan = fb_desc->pitch;
	prop->visual = VISUAL_RGB_5_6_5_LE;

	printf("BCM2835 framebuffer at 0x%08x (%dx%d)\n", prop->addr,
	    prop->x, prop->y);
	ret = true;
out:
	km_unmap((uintptr_t)fb_mbox, sizeof(bcm2835_mbox_t));
	return ret;
}

/**
 * @}
 */
