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

/** @addtogroup libnet
 * @{
 */

#ifndef LIBNET_NETIF_REMOTE_H_
#define LIBNET_NETIF_REMOTE_H_

#include <async.h>
#include <ipc/services.h>
#include <adt/measured_strings.h>

#include <net/device.h>
#include <net/packet.h>

extern int netif_get_addr_req_remote(int, device_id_t, measured_string_t **,
    char **);
extern int netif_probe_req_remote(int, device_id_t, int, int);
extern int netif_send_msg_remote(int, device_id_t, packet_t, services_t);
extern int netif_start_req_remote(int, device_id_t);
extern int netif_stop_req_remote(int, device_id_t);
extern int netif_stats_req_remote(int, device_id_t, device_stats_t *);
extern int netif_bind_service_remote(services_t, device_id_t, services_t,
    async_client_conn_t);

#endif

/** @}
 */
