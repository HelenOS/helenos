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

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <inet/addr.h>
#include "dns_type.h"

extern inet_addr_t dns_server_addr;

extern errno_t transport_init(void);
extern void transport_fini(void);
extern errno_t dns_request(dns_message_t *, dns_message_t **);

#endif

/** @}
 */
