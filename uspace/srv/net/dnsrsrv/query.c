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
#include <io/log.h>
#include <mem.h>
#include <stdlib.h>
#include <str.h>
#include "dns_msg.h"
#include "dns_std.h"
#include "dns_type.h"
#include "query.h"
#include "transport.h"

static uint16_t msg_id;

static errno_t dns_name_query(const char *name, dns_qtype_t qtype,
    dns_host_info_t *info)
{
	/* Start with the caller-provided name */
	char *sname = str_dup(name);
	if (sname == NULL)
		return ENOMEM;

	char *qname = str_dup(name);
	if (qname == NULL) {
		free(sname);
		return ENOMEM;
	}

	dns_question_t *question = calloc(1, sizeof(dns_question_t));
	if (question == NULL) {
		free(qname);
		free(sname);
		return ENOMEM;
	}

	question->qname = qname;
	question->qtype = qtype;
	question->qclass = DC_IN;

	dns_message_t *msg = dns_message_new();
	if (msg == NULL) {
		free(question);
		free(qname);
		free(sname);
		return ENOMEM;
	}

	msg->id = msg_id++;
	msg->qr = QR_QUERY;
	msg->opcode = OPC_QUERY;
	msg->aa = false;
	msg->tc = false;
	msg->rd = true;
	msg->ra = false;

	list_append(&question->msg, &msg->question);

	log_msg(LOG_DEFAULT, LVL_DEBUG, "dns_name_query: send DNS request");
	dns_message_t *amsg;
	errno_t rc = dns_request(msg, &amsg);
	if (rc != EOK) {
		dns_message_destroy(msg);
		free(sname);
		return rc;
	}

	list_foreach(amsg->answer, msg, dns_rr_t, rr) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, " - '%s' %u/%u, dsize %zu",
		    rr->name, rr->rtype, rr->rclass, rr->rdata_size);

		if ((rr->rtype == DTYPE_CNAME) && (rr->rclass == DC_IN) &&
		    (str_cmp(rr->name, sname) == 0)) {

			log_msg(LOG_DEFAULT, LVL_DEBUG, "decode cname (%p, %zu, %zu)",
			    amsg->pdu.data, amsg->pdu.size, rr->roff);

			char *cname;
			size_t eoff;
			rc = dns_name_decode(&amsg->pdu, rr->roff, &cname, &eoff);
			if (rc != EOK) {
				assert((rc == EINVAL) || (rc == ENOMEM));

				log_msg(LOG_DEFAULT, LVL_DEBUG, "error decoding cname");

				dns_message_destroy(msg);
				dns_message_destroy(amsg);
				free(sname);

				return rc;
			}

			log_msg(LOG_DEFAULT, LVL_DEBUG, "name = '%s' "
			    "cname = '%s'", sname, cname);

			/* Continue looking for the more canonical name */
			free(sname);
			sname = cname;
		}

		if ((qtype == DTYPE_A) && (rr->rtype == DTYPE_A) &&
		    (rr->rclass == DC_IN) && (rr->rdata_size == sizeof(addr32_t)) &&
		    (str_cmp(rr->name, sname) == 0)) {

			info->cname = str_dup(rr->name);
			if (info->cname == NULL) {
				dns_message_destroy(msg);
				dns_message_destroy(amsg);
				free(sname);

				return ENOMEM;
			}

			inet_addr_set(dns_uint32_t_decode(rr->rdata, rr->rdata_size),
			    &info->addr);

			dns_message_destroy(msg);
			dns_message_destroy(amsg);
			free(sname);

			return EOK;
		}

		if ((qtype == DTYPE_AAAA) && (rr->rtype == DTYPE_AAAA) &&
		    (rr->rclass == DC_IN) && (rr->rdata_size == sizeof(addr128_t)) &&
		    (str_cmp(rr->name, sname) == 0)) {

			info->cname = str_dup(rr->name);
			if (info->cname == NULL) {
				dns_message_destroy(msg);
				dns_message_destroy(amsg);
				free(sname);

				return ENOMEM;
			}

			addr128_t addr;
			dns_addr128_t_decode(rr->rdata, rr->rdata_size, addr);

			inet_addr_set6(addr, &info->addr);

			dns_message_destroy(msg);
			dns_message_destroy(amsg);
			free(sname);

			return EOK;
		}
	}

	log_msg(LOG_DEFAULT, LVL_DEBUG, "'%s' not resolved, fail", sname);

	dns_message_destroy(msg);
	dns_message_destroy(amsg);
	free(sname);

	return EIO;
}

errno_t dns_name2host(const char *name, dns_host_info_t **rinfo, ip_ver_t ver)
{
	dns_host_info_t *info = calloc(1, sizeof(dns_host_info_t));
	if (info == NULL)
		return ENOMEM;

	errno_t rc;

	switch (ver) {
	case ip_any:
		rc = dns_name_query(name, DTYPE_AAAA, info);

		if (rc != EOK)
			rc = dns_name_query(name, DTYPE_A, info);

		break;
	case ip_v4:
		rc = dns_name_query(name, DTYPE_A, info);
		break;
	case ip_v6:
		rc = dns_name_query(name, DTYPE_AAAA, info);
		break;
	default:
		rc = EINVAL;
	}

	if (rc == EOK)
		*rinfo = info;
	else
		free(info);

	return rc;
}

void dns_hostinfo_destroy(dns_host_info_t *info)
{
	free(info->cname);
	free(info);
}

/** @}
 */
