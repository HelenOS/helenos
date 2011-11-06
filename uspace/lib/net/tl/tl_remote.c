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

#include <tl_remote.h>
#include <generic.h>
#include <packet_client.h>
#include <ipc/services.h>
#include <ipc/tl.h>
#include <net/device.h>
#include <net/packet.h>
#include <async.h>

/** Notify the remote transport layer modules about the received packet/s.
 *
 * @param[in] sess      Transport layer module session.
 * @param[in] device_id Device identifier.
 * @param[in] packet    Received packet or the received packet queue.
 *                      The packet queue is used to carry a fragmented
 *                      datagram. The first packet contains the headers,
 *                      the others contain only data.
 * @param[in] target    Target transport layer module service to be
 *                      delivered to.
 * @param[in] error     Packet error reporting service. Prefixes the
 *                      received packet.
 *
 * @return EOK on success.
 *
 */
int tl_received_msg(async_sess_t *sess, nic_device_id_t device_id, packet_t *packet,
    services_t target, services_t error)
{
	return generic_received_msg_remote(sess, NET_TL_RECEIVED, device_id,
	    packet_get_id(packet), target, error);
}

/** @}
 */
