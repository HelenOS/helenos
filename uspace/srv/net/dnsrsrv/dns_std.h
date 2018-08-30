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

/** @addtogroup dnsrsrv
 * @{
 */
/**
 * @file DNS Standard definitions.
 *
 * From RFC 1035 Domain Names - Implementation and Specification
 */

#ifndef DNS_STD_H
#define DNS_STD_H

#include <stdint.h>

/** From 2.3.4. Size Limits */
enum dns_limits {
	DNS_LABEL_MAX_SIZE	= 63,
	DNS_NAME_MAX_SIZE	= 255,
	DNS_UDP_MSG_MAX_SIZE	= 512
};

typedef enum dns_qtype {
	DTYPE_A		= 1,
	DTYPE_NS	= 2,
	DTYPE_MD	= 3,
	DTYPE_MF	= 4,
	DTYPE_CNAME	= 5,
	DTYPE_SOA	= 6,
	DTYPE_MB	= 7,
	DTYPE_MG	= 8,
	DTYPE_MR	= 9,
	DTYPE_NULL	= 10,
	DTYPE_WKS	= 11,
	DTYPE_PTR	= 12,
	DTYPE_HINFO	= 13,
	DTYPE_MINFO	= 14,
	DTYPE_MX	= 15,
	DTYPE_TXT	= 16,
	DTYPE_AAAA	= 28,
	DQTYPE_AXFR	= 252,
	DQTYPE_MAILB	= 253,
	DQTYPE_MAILA	= 254,
	DQTYPE_ALL	= 255
} dns_type_t, dns_qtype_t;

typedef enum dns_qclass {
	/** Internet */
	DC_IN		= 1,
	/** CSNET */
	DC_CS		= 2,
	/** CHAOS */
	DC_CH		= 3,
	/** Hesiod */
	DC_HS		= 4,
	/** Any class */
	DQC_ANY		= 255
} dns_class_t, dns_qclass_t;

typedef struct {
	/** Identifier assigned by the query originator */
	uint16_t id;
	/** QR, Opcode, AA, TC,RD, RA, Z, Rcode */
	uint16_t opbits;
	/** Number of entries in query section */
	uint16_t qd_count;
	/** Number of RRs in the answer section */
	uint16_t an_count;
	/** Number of name server RRs in the authority records section */
	uint16_t ns_count;
	/** Number of RRs in the additional records section */
	uint16_t ar_count;
} dns_header_t;

/** Bits in dns_header_t.opbits.
 *
 * Note that bit numbers in RFC 1035 are reversed (0 is the most significant)
 * but we use the standard notation (0 is the least significant).
 */
enum dns_opbits {
	OPB_QR 		= 15,
	OPB_OPCODE_h	= 14,
	OPB_OPCODE_l	= 11,
	OPB_AA		= 10,
	OPB_TC		= 9,
	OPB_RD		= 8,
	OPB_RA		= 7,
	OPB_Z_h		= 6,
	OPB_Z_l		= 4,
	OPB_RCODE_h	= 3,
	OPB_RCODE_l	= 0
};

typedef enum dns_query_response {
	QR_QUERY	= 0,
	QR_RESPONSE	= 1
} dns_query_response_t;

typedef enum dns_opcode {
	OPC_QUERY	= 0,
	OPC_IQUERY	= 1,
	OPC_STATUS	= 2
} dns_opcode_t;

typedef enum dns_rcode {
	RC_OK		= 0,
	RC_FMT_ERR	= 1,
	RC_SRV_FAIL	= 2,
	RC_NAME_ERR	= 3,
	RC_NOT_IMPL	= 4,
	RC_REFUSED	= 5
} dns_rcode_t;

#endif

/** @}
 */
