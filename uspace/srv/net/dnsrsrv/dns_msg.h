/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup dnsrsrv
 * @{
 */
/**
 * @file
 */

#ifndef DNS_MSG_H
#define DNS_MSG_H

#include <adt/list.h>
#include <stdbool.h>
#include <stdint.h>
#include <inet/addr.h>
#include "dns_std.h"
#include "dns_type.h"

extern errno_t dns_message_encode(dns_message_t *, void **, size_t *);
extern errno_t dns_message_decode(void *, size_t, dns_message_t **);
extern dns_message_t *dns_message_new(void);
extern void dns_message_destroy(dns_message_t *);
extern errno_t dns_name_decode(dns_pdu_t *, size_t, char **, size_t *);
extern uint32_t dns_uint32_t_decode(uint8_t *, size_t);
extern void dns_addr128_t_decode(uint8_t *, size_t, addr128_t);

#endif

/** @}
 */
