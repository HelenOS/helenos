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

#include <errno.h>
#include <mem.h>
#include <stdlib.h>
#include <str.h>

#include "dns_msg.h"
#include "dns_std.h"
#include "dns_type.h"
#include "query.h"
#include "transport.h"

static uint16_t msg_id;

#include <stdio.h>
int dns_name2host(const char *name, dns_host_info_t **rinfo)
{
	dns_message_t msg;
	dns_message_t *amsg;
	dns_question_t question;
	dns_host_info_t *info;
	int rc;

	question.qname = (char *)name;
	question.qtype = DTYPE_A;
	question.qclass = DC_IN;

	memset(&msg, 0, sizeof(msg));

	list_initialize(&msg.question);
	list_append(&question.msg, &msg.question);

	msg.id = msg_id++;
	msg.qr = QR_QUERY;
	msg.opcode = OPC_QUERY;
	msg.aa = false;
	msg.tc = false;
	msg.rd = true;
	msg.ra = false;

	rc = dns_request(&msg, &amsg);
	if (rc != EOK)
		return rc;

	list_foreach(amsg->answer, link) {
		dns_rr_t *rr = list_get_instance(link, dns_rr_t, msg);

		printf(" - '%s' %u/%u, dsize %u\n",
			rr->name, rr->rtype, rr->rclass, rr->rdata_size);

		if (rr->rtype == DTYPE_A && rr->rclass == DC_IN &&
			rr->rdata_size == sizeof(uint32_t)) {

			info = calloc(1, sizeof(dns_host_info_t));
			if (info == NULL) {
				dns_message_destroy(amsg);
				return ENOMEM;
			}

			info->name = str_dup(rr->name);
			info->addr.ipv4 = dns_uint32_t_decode(rr->rdata, rr->rdata_size);
			printf("info->addr = %x\n", info->addr.ipv4);

			dns_message_destroy(amsg);
			*rinfo = info;
			return EOK;
		}
	}

	dns_message_destroy(amsg);
	printf("no A/IN found, fail\n");

	return EIO;
}

void dns_hostinfo_destroy(dns_host_info_t *info)
{
	free(info->name);
	free(info);
}

/** @}
 */
