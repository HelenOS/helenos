/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup netif
 * @{
 */

#ifndef __NET_NETIF_INTERFACE_H__
#define __NET_NETIF_INTERFACE_H__

#ifdef CONFIG_NETIF_NIL_BUNDLE

#include <netif_local.h>
#include <netif_nil_bundle.h>
#include <packet/packet_server.h>

#define netif_module_message    netif_nil_module_message
#define netif_module_start      netif_nil_module_start
#define netif_get_addr_req      netif_get_addr_req_local
#define netif_probe_req         netif_probe_req_local
#define netif_send_msg          netif_send_msg_local
#define netif_start_req         netif_start_req_local
#define netif_stop_req          netif_stop_req_local
#define netif_stats_req         netif_stats_req_local
#define netif_bind_service      netif_bind_service_local

#else /* CONFIG_NETIF_NIL_BUNDLE */

#include <netif_remote.h>
#include <packet/packet_client.h>

#define netif_module_message    netif_module_message_standalone
#define netif_module_start      netif_module_start_standalone
#define netif_get_addr_req      netif_get_addr_req_remote
#define netif_probe_req         netif_probe_req_remote
#define netif_send_msg          netif_send_msg_remote
#define netif_start_req         netif_start_req_remote
#define netif_stop_req          netif_stop_req_remote
#define netif_stats_req         netif_stats_req_remote
#define netif_bind_service      netif_bind_service_remote

#endif /* CONFIG_NETIF_NIL_BUNDLE */

#endif

/** @}
 */
