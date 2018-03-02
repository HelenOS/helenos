/*
 * Copyright (c) 2013 Jiri Svoboda
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
#include <io/log.h>
#include <macros.h>
#include <stdint.h>
#include <stdlib.h>
#include <str.h>

#include "dns_msg.h"
#include "dns_std.h"

#define NAME  "dnsres"

static uint16_t dns_uint16_t_decode(uint8_t *, size_t);

/** Extend dynamically allocated string with suffix.
 *
 * @a *dstr points to a dynamically alocated buffer containing a string.
 * Reallocate this buffer so that concatenation of @a *dstr and @a suff can
 * fit in and append @a suff.
 */
static errno_t dns_dstr_ext(char **dstr, const char *suff)
{
	size_t s1, s2;
	size_t nsize;
	char *nstr;

	if (*dstr == NULL) {
		*dstr = str_dup(suff);
		if (*dstr == NULL)
			return ENOMEM;
		return EOK;
	}

	s1 = str_size(*dstr);
	s2 = str_size(suff);
	nsize = s1 + s2 + 1;

	nstr = realloc(*dstr, nsize);
	if (nstr == NULL)
		return ENOMEM;

	str_cpy(nstr + s1, nsize - s1, suff);

	*dstr = nstr;
	return EOK;
}

/** Encode DNS name.
 *
 * Encode DNS name or measure the size of encoded name (with @a buf NULL,
 * and @a buf_size 0).
 *
 * @param name		String to encode
 * @param buf		Buffer or NULL
 * @param buf_size      Buffer size or 0 if @a buf is NULL
 * @param act_size	Place to store actual encoded size
 */
static errno_t dns_name_encode(char *name, uint8_t *buf, size_t buf_size,
    size_t *act_size)
{
	size_t off;
	wchar_t c;
	size_t lsize;
	size_t pi, di;

	pi = 0;
	di = 1;
	off = 0;

	lsize = 0;
	while (true) {
		c = str_decode(name, &off, STR_NO_LIMIT);
		if (c >= 127) {
			/* Non-ASCII character */
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Non-ascii character");
			return EINVAL;
		}

		if (c == '.' || c == '\0') {
			/* Empty string, starting with period or two consecutive periods. */
			if (lsize == 0) {
				log_msg(LOG_DEFAULT, LVL_DEBUG, "Empty token");
				return EINVAL;
			}

			if (lsize > DNS_LABEL_MAX_SIZE) {
				/* Label too long */
				log_msg(LOG_DEFAULT, LVL_DEBUG, "Label too long");
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

/** Decode DNS name.
 *
 * @param pdu	PDU from which we are decoding
 * @param boff	Starting offset within PDU
 * @param rname	Place to return dynamically allocated string
 * @param eoff	Place to store end offset (offset after last decoded byte)
 */
errno_t dns_name_decode(dns_pdu_t *pdu, size_t boff, char **rname,
    size_t *eoff)
{
	uint8_t *bp;
	size_t bsize;
	size_t lsize;
	size_t i;
	size_t ptr;
	size_t eptr;
	char *name;
	char dbuf[2];
	errno_t rc;
	bool first;

	name = NULL;

	if (boff > pdu->size)
		return EINVAL;

	bp = pdu->data + boff;
	bsize = min(pdu->size - boff, DNS_NAME_MAX_SIZE);
	first = true;
	*eoff = 0;

	while (true) {
		if (bsize == 0) {
			rc = EINVAL;
			goto error;
		}

		lsize = *bp;
		++bp;
		--bsize;

		if (lsize == 0)
			break;

		if ((lsize & 0xc0) == 0xc0) {
			/* Pointer */
			if (bsize < 1) {
				log_msg(LOG_DEFAULT, LVL_DEBUG, "Pointer- bsize < 1");
				rc = EINVAL;
				goto error;
			}

			ptr = dns_uint16_t_decode(bp - 1, bsize) & 0x3fff;
			++bp;
			--bsize;

			if (ptr >= (size_t)(bp - pdu->data)) {
				log_msg(LOG_DEFAULT, LVL_DEBUG,
				    "Pointer- forward ref %zu, pos=%zu",
				    ptr, (size_t)(bp - pdu->data));
				/* Forward reference */
				rc = EINVAL;
				goto error;
			}

			/*
			 * Make sure we will not decode any byte twice.
			 * XXX Is assumption correct?
			 */
			eptr = bp - pdu->data;
			/*
			 * This is where encoded name ends in terms where
			 * the message continues
			 */
			if (*eoff == 0)
				*eoff = eptr;

			bp = pdu->data + ptr;
			bsize = eptr - ptr;
			continue;
		}

		if (lsize > bsize) {
			rc = EINVAL;
			goto error;
		}

		if (!first) {
			rc = dns_dstr_ext(&name, ".");
			if (rc != EOK) {
				rc = ENOMEM;
				goto error;
			}
		}

		for (i = 0; i < lsize; i++) {
			if (*bp < 32 || *bp >= 127) {
				rc = EINVAL;
				goto error;
			}

			dbuf[0] = *bp;
			dbuf[1] = '\0';

			rc = dns_dstr_ext(&name, dbuf);
			if (rc != EOK) {
				rc = ENOMEM;
				goto error;
			}
			++bp;
			--bsize;
		}

		first = false;
	}

	*rname = name;
	if (*eoff == 0)
		*eoff = bp - pdu->data;
	return EOK;
error:
	free(name);
	return rc;
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
uint32_t dns_uint32_t_decode(uint8_t *buf, size_t buf_size)
{
	assert(buf_size >= 4);

	uint32_t w = ((uint32_t) buf[0] << 24) +
	    ((uint32_t) buf[1] << 16) +
	    ((uint32_t) buf[2] << 8) +
	    buf[3];

	return w;
}

/** Decode unaligned big-endian 128-bit integer */
void dns_addr128_t_decode(uint8_t *buf, size_t buf_size, addr128_t addr)
{
	assert(buf_size >= 16);

	addr128_t_be2host(buf, addr);
}

/** Encode DNS question.
 *
 * Encode DNS question or measure the size of encoded question (with @a buf NULL,
 * and @a buf_size 0).
 *
 * @param question	Question to encode
 * @param buf		Buffer or NULL
 * @param buf_size      Buffer size or 0 if @a buf is NULL
 * @param act_size	Place to store actual encoded size
 */
static errno_t dns_question_encode(dns_question_t *question, uint8_t *buf,
    size_t buf_size, size_t *act_size)
{
	size_t name_size;
	size_t di;
	errno_t rc;

	rc = dns_name_encode(question->qname, buf, buf_size, &name_size);
	if (rc != EOK)
		return rc;

	*act_size = name_size + sizeof(uint16_t) + sizeof(uint16_t);
	if (buf == NULL)
		return EOK;

	di = name_size;

	dns_uint16_t_encode(question->qtype, buf + di, buf_size - di);
	di += sizeof(uint16_t);

	dns_uint16_t_encode(question->qclass, buf + di, buf_size - di);
	di += sizeof(uint16_t);

	return EOK;
}

/** Decode DNS question.
 *
 * @param pdu		PDU from which we are decoding
 * @param boff		Starting offset within PDU
 * @param rquestion	Place to return dynamically allocated question
 * @param eoff		Place to store end offset (offset after last decoded byte)
 */
static errno_t dns_question_decode(dns_pdu_t *pdu, size_t boff,
    dns_question_t **rquestion, size_t *eoff)
{
	dns_question_t *question;
	size_t name_eoff;
	errno_t rc;

	question = calloc(1, sizeof (dns_question_t));
	if (question == NULL)
		return ENOMEM;

	rc = dns_name_decode(pdu, boff, &question->qname, &name_eoff);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Error decoding name");
		free(question);
		return ENOMEM;
	}

	if (name_eoff + 2 * sizeof(uint16_t) > pdu->size) {
		free(question);
		return EINVAL;
	}

	question->qtype = dns_uint16_t_decode(pdu->data + name_eoff,
	    pdu->size - name_eoff);
	question->qclass = dns_uint16_t_decode(pdu->data + sizeof(uint16_t)
	    + name_eoff, pdu->size - sizeof(uint16_t) - name_eoff);
	*eoff = name_eoff + 2 * sizeof(uint16_t);

	*rquestion = question;
	return EOK;
}

/** Decode DNS resource record.
 *
 * @param pdu		PDU from which we are decoding
 * @param boff		Starting offset within PDU
 * @param retrr		Place to return dynamically allocated resource record
 * @param eoff		Place to store end offset (offset after last decoded byte)
 */
static errno_t dns_rr_decode(dns_pdu_t *pdu, size_t boff, dns_rr_t **retrr,
    size_t *eoff)
{
	dns_rr_t *rr;
	size_t name_eoff;
	uint8_t *bp;
	size_t bsz;
	size_t rdlength;
	errno_t rc;

	rr = calloc(1, sizeof(dns_rr_t));
	if (rr == NULL)
		return ENOMEM;

	rc = dns_name_decode(pdu, boff, &rr->name, &name_eoff);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "dns_rr_decode: error decoding name");
		free(rr);
		return ENOMEM;
	}

	if (name_eoff + 2 * sizeof(uint16_t) > pdu->size) {
		free(rr->name);
		free(rr);
		log_msg(LOG_DEFAULT, LVL_DEBUG, "dns_rr_decode: error name_off");
		return EINVAL;
	}

	bp = pdu->data + name_eoff;
	bsz = pdu->size - name_eoff;

	if (bsz < 3 * sizeof(uint16_t) + sizeof(uint32_t)) {
		free(rr->name);
		free(rr);
		log_msg(LOG_DEFAULT, LVL_DEBUG, "dns_rr_decode: error bsz");
		return EINVAL;
	}

	rr->rtype = dns_uint16_t_decode(bp, bsz);
	bp += sizeof(uint16_t);
	bsz -= sizeof(uint16_t);

	rr->rclass = dns_uint16_t_decode(bp, bsz);
	bp += sizeof(uint16_t);
	bsz -= sizeof(uint16_t);

	rr->ttl = dns_uint32_t_decode(bp, bsz);
	bp += sizeof(uint32_t);
	bsz -= sizeof(uint32_t);

	log_msg(LOG_DEFAULT, LVL_DEBUG2, "dns_rr_decode: rtype=0x%x, rclass=0x%x, ttl=0x%x",
	    rr->rtype, rr->rclass, rr->ttl );

	rdlength = dns_uint16_t_decode(bp, bsz);
	bp += sizeof(uint16_t);
	bsz -= sizeof(uint16_t);

	if (rdlength > bsz) {
		free(rr->name);
		free(rr);
		log_msg(LOG_DEFAULT, LVL_DEBUG, "dns_rr_decode: Error rdlength %zu > bsz %zu", rdlength, bsz);
		return EINVAL;
	}

	rr->rdata_size = rdlength;
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "dns_rr_decode: rdlength=%zu", rdlength);
	rr->rdata = calloc(1, rdlength);
	if (rr->rdata == NULL) {
		free(rr->name);
		free(rr);
		log_msg(LOG_DEFAULT, LVL_DEBUG, "dns_rr_decode: Error memory");
		return ENOMEM;
	}

	memcpy(rr->rdata, bp, rdlength);
	rr->roff = bp - pdu->data;
	bp += rdlength;
	bsz -= rdlength;

	*eoff = bp - pdu->data;
	*retrr = rr;
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "dns_rr_decode: done");
	return EOK;
}

/** Encode DNS message.
 *
 * @param msg	Message
 * @param rdata	Place to store encoded data pointer
 * @param rsize	Place to store encoded data size
 *
 * @return 	EOK on success, EINVAL if message contains invalid data,
 *		ENOMEM if out of memory
 */
errno_t dns_message_encode(dns_message_t *msg, void **rdata, size_t *rsize)
{
	uint8_t *data;
	size_t size;
	dns_header_t hdr;
	size_t q_size = 0;
	size_t di;
	errno_t rc;

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

	list_foreach(msg->question, msg, dns_question_t, q) {
		rc = dns_question_encode(q, NULL, 0, &q_size);
		if (rc != EOK)
			return rc;

		assert(q_size > 0);

		size += q_size;
	}

	data = calloc(1, size);
	if (data == NULL)
		return ENOMEM;

	memcpy(data, &hdr, sizeof(dns_header_t));
	di = sizeof(dns_header_t);

	list_foreach(msg->question, msg, dns_question_t, q) {
		rc = dns_question_encode(q, data + di, size - di, &q_size);
		if (rc != EOK) {
			assert(rc == ENOMEM || rc == EINVAL);
			free(data);
			return rc;
		}

		assert(q_size > 0);

		di += q_size;
	}

	*rdata = data;
	*rsize = size;
	return EOK;
}

/** Decode DNS message.
 *
 * @param data	Encoded PDU data
 * @param size	Encoded PDU size
 * @param rmsg	Place to store pointer to decoded message
 *
 * @return	EOK on success, EINVAL if message contains invalid data,
 * 		ENOMEM if out of memory
 */
errno_t dns_message_decode(void *data, size_t size, dns_message_t **rmsg)
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
	errno_t rc;

	msg = dns_message_new();
	if (msg == NULL)
		return ENOMEM;

	if (size < sizeof(dns_header_t)) {
		rc = EINVAL;
		goto error;
	}

	/* Store a copy of raw message data for string decompression */

	msg->pdu.data = malloc(size);
	if (msg->pdu.data == NULL) {
		rc = ENOMEM;
		goto error;
	}

	memcpy(msg->pdu.data, data, size);
	msg->pdu.size = size;
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "dns_message_decode: pdu->data = %p, "
	    "pdu->size=%zu", msg->pdu.data, msg->pdu.size);

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

	doff = sizeof(dns_header_t);

	qd_count = uint16_t_be2host(hdr->qd_count);
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "qd_count=%zu", qd_count);


	for (i = 0; i < qd_count; i++) {
		rc = dns_question_decode(&msg->pdu, doff, &question, &field_eoff);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "error decoding question");
			goto error;
		}

		list_append(&question->msg, &msg->question);
		doff = field_eoff;
	}

	an_count = uint16_t_be2host(hdr->an_count);
	log_msg(LOG_DEFAULT, LVL_DEBUG2, "an_count=%zu", an_count);

	for (i = 0; i < an_count; i++) {
		rc = dns_rr_decode(&msg->pdu, doff, &rr, &field_eoff);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_DEBUG, "Error decoding answer");
			goto error;
		}

		list_append(&rr->msg, &msg->answer);
		doff = field_eoff;
	}

	*rmsg = msg;
	return EOK;
error:
	dns_message_destroy(msg);
	return rc;
}

/** Destroy question. */
static void dns_question_destroy(dns_question_t *question)
{
	free(question->qname);
	free(question);
}

/** Destroy resource record. */
static void dns_rr_destroy(dns_rr_t *rr)
{
	free(rr->name);
	free(rr->rdata);
	free(rr);
}

/** Create new empty message. */
dns_message_t *dns_message_new(void)
{
	dns_message_t *msg;

	msg = calloc(1, sizeof(dns_message_t));
	if (msg == NULL)
		return NULL;

	list_initialize(&msg->question);
	list_initialize(&msg->answer);
	list_initialize(&msg->authority);
	list_initialize(&msg->additional);

	return msg;
}

/** Destroy message. */
void dns_message_destroy(dns_message_t *msg)
{
	link_t *link;
	dns_question_t *question;
	dns_rr_t *rr;

	while (!list_empty(&msg->question)) {
		link = list_first(&msg->question);
		question = list_get_instance(link, dns_question_t, msg);
		list_remove(&question->msg);
		dns_question_destroy(question);
	}

	while (!list_empty(&msg->answer)) {
		link = list_first(&msg->answer);
		rr = list_get_instance(link, dns_rr_t, msg);
		list_remove(&rr->msg);
		dns_rr_destroy(rr);
	}

	while (!list_empty(&msg->authority)) {
		link = list_first(&msg->authority);
		rr = list_get_instance(link, dns_rr_t, msg);
		list_remove(&rr->msg);
		dns_rr_destroy(rr);
	}

	while (!list_empty(&msg->additional)) {
		link = list_first(&msg->additional);
		rr = list_get_instance(link, dns_rr_t, msg);
		list_remove(&rr->msg);
		dns_rr_destroy(rr);
	}

	free(msg->pdu.data);
	free(msg);
}

/** @}
 */
