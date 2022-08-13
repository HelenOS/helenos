/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup dnsrsrv
 * @{
 */
/**
 * @file
 */

#ifndef DNS_TYPE_H
#define DNS_TYPE_H

#include <adt/list.h>
#include <inet/inet.h>
#include <inet/addr.h>
#include <stdbool.h>
#include <stdint.h>
#include "dns_std.h"

/** Encoded DNS PDU */
typedef struct {
	/** Encoded PDU data */
	uint8_t *data;
	/** Encoded PDU size */
	size_t size;
} dns_pdu_t;

/** DNS message */
typedef struct {
	/* Encoded PDU */
	dns_pdu_t pdu;

	/** Identifier */
	uint16_t id;
	/** Query or Response */
	dns_query_response_t qr;
	/** Opcode */
	dns_opcode_t opcode;
	/** Authoritative Answer */
	bool aa;
	/** TrunCation */
	bool tc;
	/** Recursion Desired */
	bool rd;
	/** Recursion Available */
	bool ra;
	/** Response Code */
	dns_rcode_t rcode;

	list_t question; /* of dns_question_t */
	list_t answer; /* of dns_rr_t */
	list_t authority; /* of dns_rr_t */
	list_t additional; /* of dns_rr_t */
} dns_message_t;

/** Unencoded DNS message question section */
typedef struct {
	link_t msg;
	/** Domain name in text format (dot notation) */
	char *qname;
	/** Query type */
	dns_qtype_t qtype;
	/** Query class */
	dns_qclass_t qclass;
} dns_question_t;

/** Unencoded DNS resource record */
typedef struct {
	link_t msg;
	/** Domain name */
	char *name;
	/** RR type */
	dns_type_t rtype;
	/** Class of data */
	dns_class_t rclass;
	/** Time to live */
	uint32_t ttl;

	/** Resource data */
	void *rdata;
	/** Number of bytes in @c *rdata */
	size_t rdata_size;
	/** Offset in the raw message */
	size_t roff;
} dns_rr_t;

/** Host information */
typedef struct {
	/** Host name */
	char *cname;
	/** Host address */
	inet_addr_t addr;
} dns_host_info_t;

typedef struct {
} dnsr_client_t;

#endif

/** @}
 */
