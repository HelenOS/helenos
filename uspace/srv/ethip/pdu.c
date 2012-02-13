/*
 * Copyright (c) 2012 Jiri Svoboda
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

/** @addtogroup inet
 * @{
 */
/**
 * @file
 * @brief
 */

#include <byteorder.h>
#include <errno.h>
#include <io/log.h>
#include <macros.h>
#include <mem.h>
#include <stdlib.h>

#include "ethip.h"
#include "std.h"
#include "pdu.h"

static void mac48_encode(mac48_addr_t *addr, void *buf);
static void mac48_decode(void *data, mac48_addr_t *addr);

#define MAC48_BYTES 6

/** Encode Ethernet PDU. */
int eth_pdu_encode(eth_frame_t *frame, void **rdata, size_t *rsize)
{
	void *data;
	size_t size;
	eth_header_t *hdr;

	size = max(sizeof(eth_header_t) + frame->size, ETH_FRAME_MIN_SIZE);

	data = calloc(size, 1);
	if (data == NULL)
		return ENOMEM;

	hdr = (eth_header_t *)data;
	mac48_encode(&frame->src, hdr->src);
	mac48_encode(&frame->dest, hdr->dest);
	hdr->etype_len = host2uint16_t_be(frame->etype_len);

	memcpy((uint8_t *)data + sizeof(eth_header_t), frame->data,
	    frame->size);

	*rdata = data;
	*rsize = size;
	return EOK;
}

#include <stdio.h>
/** Decode Ethernet PDU. */
int eth_pdu_decode(void *data, size_t size, eth_frame_t *frame)
{
	eth_header_t *hdr;

	log_msg(LVL_DEBUG, "eth_pdu_decode()");

	if (size < sizeof(eth_header_t)) {
		log_msg(LVL_DEBUG, "PDU too short (%zu)", size);
		return EINVAL;
	}

	hdr = (eth_header_t *)data;

	frame->size = size - sizeof(eth_header_t);
	frame->data = calloc(frame->size, 1);
	if (frame->data == NULL)
		return ENOMEM;

	mac48_decode(hdr->src, &frame->src);
	mac48_decode(hdr->dest, &frame->dest);
	frame->etype_len = uint16_t_be2host(hdr->etype_len);

	memcpy(frame->data, (uint8_t *)data + sizeof(eth_header_t),
	    frame->size);

	log_msg(LVL_DEBUG, "Ethernet frame src=%llx dest=%llx etype=%x",
	    frame->src, frame->dest, frame->etype_len);
	log_msg(LVL_DEBUG, "Ethernet frame payload (%zu bytes)", frame->size);
	size_t i;
	for (i = 0; i < frame->size; i++) {
		printf("%02x ", ((uint8_t *)(frame->data))[i]);
	}
	printf("\n");

	return EOK;
}

static void mac48_encode(mac48_addr_t *addr, void *buf)
{
	uint64_t val;
	uint8_t *bbuf = (uint8_t *)buf;
	int i;

	val = addr->addr;
	for (i = 0; i < MAC48_BYTES; i++)
		bbuf[i] = (val >> (8 * (MAC48_BYTES - i - 1))) & 0xff;
}

static void mac48_decode(void *data, mac48_addr_t *addr)
{
	uint64_t val;
	uint8_t *bdata = (uint8_t *)data;
	int i;

	val = 0;
	for (i = 0; i < MAC48_BYTES; i++)
		val |= (uint64_t)bdata[i] << (8 * (MAC48_BYTES - i - 1));

	addr->addr = val;
}

/** @}
 */
