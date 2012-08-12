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

/** @addtogroup dnsres
 * @{
 */
/**
 * @file
 */

#include <bitops.h>
#include <byteorder.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <str.h>

#include "dns_msg.h"
#include "dns_std.h"

#define NAME  "dnsres"

#include <stdio.h>
static int dns_name_encode(char *name, uint8_t *buf, size_t buf_size,
    size_t *act_size)
{
	size_t off;
	wchar_t c;
	size_t lsize;
	size_t pi, di;

	pi = 0;
	di = 1;
	off = 0;

	printf("dns_name_encode(name='%s', buf=%p, buf_size=%zu, act_size=%p\n",
	    name, buf, buf_size, act_size);
	lsize = 0;
	while (true) {
		printf("off=%zu\n", off);
		c = str_decode(name, &off, STR_NO_LIMIT);
		printf("c=%d\n", (int)c);
		if (c > 127) {
			/* Non-ASCII character */
			printf("non-ascii character\n");
			return EINVAL;
		}

		if (c == '.' || c == '\0') {
			/* Empty string, starting with period or two consecutive periods. */
			if (lsize == 0) {
				printf("empty token\n");
				return EINVAL;
			}

			if (lsize > DNS_LABEL_MAX_SIZE) {
				/* Label too long */
				printf("label too long\n");
				return EINVAL;
			}

			if (buf != NULL && pi < buf_size)
				buf[pi] = (uint8_t)lsize;

			lsize = 0;
			pi = di;
			++di;

			if (c == '\0')
				break;
		} else {
			if (buf != NULL && di < buf_size)
				buf[di] = c;
			++di;
			++lsize;
		}
	}

	if (buf != NULL && pi < buf_size)
		buf[pi] = 0;

	*act_size = di;
	return EOK;
}

static void dns_uint16_t_encode(uint16_t w, uint8_t *buf, size_t buf_size)
{
	if (buf != NULL && buf_size >= 1)
		buf[0] = w >> 8;

	if (buf != NULL && buf_size >= 2)
		buf[1] = w & 0xff;
}

static int dns_question_encode(dns_question_t *question, uint8_t *buf,
    size_t buf_size, size_t *act_size)
{
	size_t name_size;
	size_t di;
	int rc;

	rc = dns_name_encode(question->qname, buf, buf_size, &name_size);
	if (rc != EOK)
		return rc;

	printf("name_size=%zu\n", name_size);

	*act_size = name_size + sizeof(uint16_t) + sizeof(uint16_t);
	printf("act_size=%zu\n", *act_size);
	if (buf == NULL)
		return EOK;

	di = name_size;

	dns_uint16_t_encode(question->qtype, buf + di, buf_size - di);
	di += sizeof(uint16_t);

	dns_uint16_t_encode(question->qclass, buf + di, buf_size - di);
	di += sizeof(uint16_t);

	return EOK;
}

int dns_message_encode(dns_message_t *msg, void **rdata, size_t *rsize)
{
	uint8_t *data;
	size_t size;
	dns_header_t hdr;
	size_t q_size;
	size_t di;
	int rc;

	hdr.id = host2uint16_t_be(msg->id);

	hdr.opbits = host2uint16_t_be(
	    (msg->qr << OPB_QR) |
	    (msg->opcode << OPB_OPCODE_l) |
	    (msg->aa ? BIT_V(uint16_t, OPB_AA) : 0) |
	    (msg->tc ? BIT_V(uint16_t, OPB_TC) : 0) |
	    (msg->rd ? BIT_V(uint16_t, OPB_RD) : 0) |
	    (msg->ra ? BIT_V(uint16_t, OPB_RA) : 0) |
	    msg->rcode
	);

	hdr.qd_count = host2uint16_t_be(list_count(&msg->question));
	hdr.an_count = 0;
	hdr.ns_count = 0;
	hdr.ar_count = 0;

	size = sizeof(dns_header_t);
	printf("dns header size=%zu\n", size);

	list_foreach(msg->question, link) {
		dns_question_t *q = list_get_instance(link, dns_question_t, msg);
		rc = dns_question_encode(q, NULL, 0, &q_size);
		if (rc != EOK)
			return rc;

		printf("q_size=%zu\n", q_size);
		size += q_size;
	}

	data = calloc(1, size);
	if (data == NULL)
		return ENOMEM;

	memcpy(data, &hdr, sizeof(dns_header_t));
	di = sizeof(dns_header_t);

	list_foreach(msg->question, link) {
		dns_question_t *q = list_get_instance(link, dns_question_t, msg);
		rc = dns_question_encode(q, data + di, size - di, &q_size);
		assert(rc == EOK);

		di += q_size;
	}

	printf("-> size=%zu, di=%zu\n", size, di);
	*rdata = data;
	*rsize = size;
	return EOK;
}

/** @}
 */
