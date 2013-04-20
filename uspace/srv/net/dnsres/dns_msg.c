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
#include <macros.h>
#include <stdint.h>
#include <stdlib.h>
#include <str.h>

#include "dns_msg.h"
#include "dns_std.h"

#define NAME  "dnsres"

static uint16_t dns_uint16_t_decode(uint8_t *, size_t);

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

static int dns_name_decode(uint8_t *buf, size_t size, size_t boff, char **rname,
    size_t *eoff)
{
	uint8_t *bp;
	size_t bsize;
	size_t lsize;
	size_t i;
	size_t ptr;
	size_t eptr;

	if (boff > size)
		return EINVAL;

	bp = buf + boff;
	bsize = min(size - boff, DNS_NAME_MAX_SIZE);

	while (true) {
		if (bsize == 0) {
			return EINVAL;
		}

		lsize = *bp;
		++bp;
		--bsize;

		if (lsize == 0)
			break;

		if (bp != buf + boff + 1)
			printf(".");

		if ((lsize & 0xc0) == 0xc0) {
			printf("Pointer\n");
			/* Pointer */
			if (bsize < 1) {
				printf("Pointer- bsize < 1\n");
				return EINVAL;
			}

			ptr = dns_uint16_t_decode(bp - 1, bsize) & 0x3fff;
			if (ptr >= (size_t)(bp - buf)) {
				printf("Pointer- forward ref %u, pos=%u\n",
				    ptr, bp - buf);
				/* Forward reference */
				return EINVAL;
			}

			/*
			 * Make sure we will not decode any byte twice.
			 * XXX Is assumption correct?
			 */
			eptr = buf - bp;

			printf("ptr=%u, eptr=%u\n", ptr, eptr);
			bp = buf + ptr;
			bsize = eptr - ptr;
			continue;
		}

		if (lsize > bsize) {
			return EINVAL;
		}

		for (i = 0; i < lsize; i++) {
			printf("%c", *bp);
			++bp;
			--bsize;
		}
	}

	printf("\n");

	*eoff = bp - buf;
	return EOK;
}

/** Decode unaligned big-endian 16-bit integer */
static uint16_t dns_uint16_t_decode(uint8_t *buf, size_t buf_size)
{
	assert(buf_size >= 2);

	return ((uint16_t)buf[0] << 8) + buf[1];
}

/** Encode unaligned big-endian 16-bit integer */
static void dns_uint16_t_encode(uint16_t w, uint8_t *buf, size_t buf_size)
{
	if (buf != NULL && buf_size >= 1)
		buf[0] = w >> 8;

	if (buf != NULL && buf_size >= 2)
		buf[1] = w & 0xff;
}

/** Decode unaligned big-endian 32-bit integer */
static uint16_t dns_uint32_t_decode(uint8_t *buf, size_t buf_size)
{
	assert(buf_size >= 4);

	return ((uint32_t)buf[0] << 24) +
	    ((uint32_t)buf[1] << 16) +
	    ((uint32_t)buf[2] << 8) +
	    buf[0];
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

static int dns_question_decode(uint8_t *buf, size_t buf_size, size_t boff,
    dns_question_t **rquestion, size_t *eoff)
{
	dns_question_t *question;
	size_t name_eoff;
	int rc;

	question = calloc(1, sizeof (dns_question_t));
	if (question == NULL)
		return ENOMEM;

	printf("decode name..\n");
	rc = dns_name_decode(buf, buf_size, boff, &question->qname, &name_eoff);
	if (rc != EOK) {
		printf("error decoding name..\n");
		free(question);
		return ENOMEM;
	}

	printf("ok decoding name..\n");
	if (name_eoff + 2 * sizeof(uint16_t) > buf_size) {
		printf("name_eoff + 2 * 2 = %d >  buf_size = %d\n",
		    name_eoff + 2 * sizeof(uint16_t), buf_size);
		free(question);
		return EINVAL;
	}

	question->qtype = dns_uint16_t_decode(buf + name_eoff, buf_size - name_eoff);
	question->qclass = dns_uint16_t_decode(buf + sizeof(uint16_t) + name_eoff,
	    buf_size - sizeof(uint16_t) - name_eoff);
	*eoff = name_eoff + 2 * sizeof(uint16_t);

	*rquestion = question;
	return EOK;
}

static int dns_rr_decode(uint8_t *buf, size_t buf_size, size_t boff,
    dns_rr_t **retrr, size_t *eoff)
{
	dns_rr_t *rr;
	size_t name_eoff;
	uint8_t *bp;
	size_t bsz;
	size_t rdlength;
	int rc;

	rr = calloc(1, sizeof (dns_rr_t));
	if (rr == NULL)
		return ENOMEM;

	printf("decode name..\n");
	rc = dns_name_decode(buf, buf_size, boff, &rr->name, &name_eoff);
	if (rc != EOK) {
		printf("error decoding name..\n");
		free(rr);
		return ENOMEM;
	}

	printf("ok decoding name..\n");
	if (name_eoff + 2 * sizeof(uint16_t) > buf_size) {
		printf("name_eoff + 2 * 2 = %d >  buf_size = %d\n",
		    name_eoff + 2 * sizeof(uint16_t), buf_size);
		free(rr->name);
		free(rr);
		return EINVAL;
	}

	bp = buf + name_eoff;
	bsz = buf_size - name_eoff;

	if (bsz < 3 * sizeof(uint16_t) + sizeof(uint32_t)) {
		free(rr->name);
		free(rr);
		return EINVAL;
	}

	rr->rtype = dns_uint16_t_decode(bp, bsz);
	bp += sizeof(uint16_t); bsz -= sizeof(uint16_t);

	rr->rclass = dns_uint16_t_decode(bp, bsz);
	bp += sizeof(uint16_t); bsz -= sizeof(uint16_t);

	rr->ttl = dns_uint32_t_decode(bp, bsz);
	bp += sizeof(uint32_t); bsz -= sizeof(uint32_t);

	rdlength = dns_uint16_t_decode(bp, bsz);
	bp += sizeof(uint16_t); bsz -= sizeof(uint16_t);

	if (rdlength > bsz) {
		free(rr->name);
		free(rr);
		return EINVAL;
	}

	rr->rdata_size = rdlength;
	rr->rdata = calloc(1, sizeof(rdlength));
	if (rr->rdata == NULL) {
		free(rr->name);
		free(rr);
		return ENOMEM;
	}

	memcpy(rr->rdata, bp, rdlength);
	bp += rdlength;
	bsz -= rdlength;

	*eoff = bp - buf;
	*retrr = rr;
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

int dns_message_decode(void *data, size_t size, dns_message_t **rmsg)
{
	dns_message_t *msg;
	dns_header_t *hdr;
	size_t doff;
	size_t field_eoff;
	dns_question_t *question;
	dns_rr_t *rr;
	size_t qd_count;
	size_t an_count;
	size_t i;
	int rc;

	msg = calloc(1, sizeof(dns_message_t));
	if (msg == NULL)
		return ENOMEM;

	if (size < sizeof(dns_header_t))
		return EINVAL;

	hdr = data;

	msg->id = uint16_t_be2host(hdr->id);
	msg->qr = BIT_RANGE_EXTRACT(uint16_t, OPB_QR, OPB_QR, hdr->opbits);
	msg->opcode = BIT_RANGE_EXTRACT(uint16_t, OPB_OPCODE_h, OPB_OPCODE_l,
	    hdr->opbits);
	msg->aa = BIT_RANGE_EXTRACT(uint16_t, OPB_AA, OPB_AA, hdr->opbits);
	msg->tc = BIT_RANGE_EXTRACT(uint16_t, OPB_TC, OPB_TC, hdr->opbits);
	msg->rd = BIT_RANGE_EXTRACT(uint16_t, OPB_RD, OPB_RD, hdr->opbits);
	msg->ra = BIT_RANGE_EXTRACT(uint16_t, OPB_RA, OPB_RA, hdr->opbits);
	msg->rcode = BIT_RANGE_EXTRACT(uint16_t, OPB_RCODE_h, OPB_RCODE_l,
	    hdr->opbits);

	list_initialize(&msg->question);
	list_initialize(&msg->answer);
	list_initialize(&msg->authority);
	list_initialize(&msg->additional);

	doff = sizeof(dns_header_t);

	qd_count = uint16_t_be2host(hdr->qd_count);
	printf("qd_count = %d\n", (int)qd_count);

	for (i = 0; i < qd_count; i++) {
		printf("decode question..\n");
		rc = dns_question_decode(data, size, doff, &question, &field_eoff);
		if (rc != EOK) {
			printf("error decoding question\n");
			goto error;
		}
		printf("ok decoding question\n");

		doff = field_eoff;
	}

	an_count = uint16_t_be2host(hdr->an_count);
	printf("an_count = %d\n", an_count);

	for (i = 0; i < an_count; i++) {
		printf("decode answer..\n");
		rc = dns_rr_decode(data, size, doff, &rr, &field_eoff);
		if (rc != EOK) {
			printf("error decoding answer\n");
			goto error;
		}
		printf("ok decoding answer\n");

		doff = field_eoff;
	}

	printf("ns_count = %d\n", uint16_t_be2host(hdr->ns_count));
	printf("ar_count = %d\n", uint16_t_be2host(hdr->ar_count));

	*rmsg = msg;
	return EOK;
error:
	/* XXX Destroy message */
	return rc;
}

/** @}
 */
