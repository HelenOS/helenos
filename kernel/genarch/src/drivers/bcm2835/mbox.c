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

#include <typedefs.h>
#include <genarch/drivers/bcm2835/mbox.h>

static void mbox_write(bcm2835_mbox_t *mbox, uint8_t chan, uint32_t value)
{
	if (!mbox)
		mbox = (bcm2835_mbox_t *)BCM2835_MBOX0_ADDR;

	while (mbox->status & MBOX_STATUS_FULL) ;
	mbox->write = MBOX_COMPOSE(chan, value);
}

static uint32_t mbox_read(bcm2835_mbox_t *mbox, uint8_t chan)
{
	uint32_t msg;

	if (!mbox)
		mbox = (bcm2835_mbox_t *)BCM2835_MBOX0_ADDR;

	do {
		while (mbox->status & MBOX_STATUS_EMPTY) ;
		msg = mbox->read;
	} while (MBOX_MSG_CHAN(msg) != chan);

	return MBOX_MSG_VALUE(msg);
}

bool bcm2835_prop_get_memory(uint32_t *base, uint32_t *size)
{
	bool ret;
	ALLOC_PROP_BUFFER(req, mbox_getmem_buf_t);

	req->buf_hdr.size = sizeof(mbox_getmem_buf_t);
	req->buf_hdr.code = MBOX_PROP_CODE_REQ;
	req->tag_hdr.tag_id = TAG_GET_ARM_MEMORY;
	req->tag_hdr.buf_size = sizeof(mbox_tag_getmem_resp_t);
	req->tag_hdr.val_len = 0;
	req->zero = 0;

	mbox_write(NULL, MBOX_CHAN_PROP_A2V, KA2VC((uint32_t)req));
	mbox_read(NULL, MBOX_CHAN_PROP_A2V);

	if (req->buf_hdr.code == MBOX_PROP_CODE_RESP_OK) {
		*base = req->data.base;
		*size = req->data.size;
		ret = true;
	} else {
		ret = false;
	}

	return ret;
}

/**
 * @}
 */
