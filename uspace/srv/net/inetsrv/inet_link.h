/*
 * Copyright (c) 2024 Jiri Svoboda
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

#ifndef INET_LINK_H_
#define INET_LINK_H_

#include <inet/eth_addr.h>
#include <stddef.h>
#include <stdint.h>
#include "inetsrv.h"

extern errno_t inet_link_open(service_id_t);
extern errno_t inet_link_send_dgram(inet_link_t *, addr32_t,
    addr32_t, inet_dgram_t *, uint8_t, uint8_t, int);
extern errno_t inet_link_send_dgram6(inet_link_t *, eth_addr_t *, inet_dgram_t *,
    uint8_t, uint8_t, int);
extern inet_link_t *inet_link_get_by_id(sysarg_t);
extern inet_link_t *inet_link_get_by_svc_name(const char *);
extern errno_t inet_link_get_id_list(sysarg_t **, size_t *);
extern errno_t inet_link_discovery_start(void);
extern errno_t inet_link_autoconf(void);
extern void inet_link_autoconf_link(inet_link_cfg_info_t *);

#endif

/** @}
 */
