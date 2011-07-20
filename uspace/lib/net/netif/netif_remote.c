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

/** @file
 * Network interface module interface implementation for remote modules.
 */

#include <netif_remote.h>
#include <packet_client.h>
#include <generic.h>
#include <ipc/services.h>
#include <ipc/netif.h>
#include <net/modules.h>
#include <adt/measured_strings.h>
#include <net/packet.h>
#include <net/device.h>

/** Return the device local hardware address.
 *
 * @param[in]  sess      Network interface session.
 * @param[in]  device_id Device identifier.
 * @param[out] address   Device local hardware address.
 * @param[out] data      Address data.
 *
 * @return EOK on success.
 * @return EBADMEM if the address parameter is NULL.
 * @return ENOENT if there no such device.
 * @return Other error codes as defined for the
 *         netif_get_addr_message() function.
 *
 */
int netif_get_addr_req(async_sess_t *sess, device_id_t device_id,
    measured_string_t **address, uint8_t **data)
{
	return generic_get_addr_req(sess, NET_NETIF_GET_ADDR, device_id,
	    address, data);
}

/** Probe the existence of the device.
 *
 * @param[in] sess      Network interface session.
 * @param[in] device_id Device identifier.
 * @param[in] irq       Device interrupt number.
 * @param[in] io        Device input/output address.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the
 *         netif_probe_message().
 *
 */
int netif_probe_req(async_sess_t *sess, device_id_t device_id, int irq, void *io)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_3_0(exch, NET_NETIF_PROBE, device_id, (sysarg_t) irq,
	    (sysarg_t) io);
	async_exchange_end(exch);
	
	return rc;
}

/** Send the packet queue.
 *
 * @param[in] sess      Network interface session.
 * @param[in] device_id Device identifier.
 * @param[in] packet    Packet queue.
 * @param[in] sender    Sending module service.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the generic_send_msg()
 *         function.
 *
 */
int netif_send_msg(async_sess_t *sess, device_id_t device_id, packet_t *packet,
    services_t sender)
{
	return generic_send_msg_remote(sess, NET_NETIF_SEND, device_id,
	    packet_get_id(packet), sender, 0);
}

/** Start the device.
 *
 * @param[in] sess      Network interface session.
 * @param[in] device_id Device identifier.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the find_device()
 *         function.
 * @return Other error codes as defined for the
 *         netif_start_message() function.
 *
 */
int netif_start_req(async_sess_t *sess, device_id_t device_id)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_1_0(exch, NET_NETIF_START, device_id);
	async_exchange_end(exch);
	
	return rc;
}

/** Stop the device.
 *
 * @param[in] sess      Network interface session.
 * @param[in] device_id Device identifier.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the find_device()
 *         function.
 * @return Other error codes as defined for the
 *         netif_stop_message() function.
 *
 */
int netif_stop_req(async_sess_t *sess, device_id_t device_id)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_1_0(exch, NET_NETIF_STOP, device_id);
	async_exchange_end(exch);
	
	return rc;
}

/** Return the device usage statistics.
 *
 * @param[in]  sess      Network interface session.
 * @param[in]  device_id Device identifier.
 * @param[out] stats     Device usage statistics.
 *
 * @return EOK on success.
 *
 */
int netif_stats_req(async_sess_t *sess, device_id_t device_id,
    device_stats_t *stats)
{
	if (!stats)
		return EBADMEM;
	
	async_exch_t *exch = async_exchange_begin(sess);
	aid_t message_id = async_send_1(exch, NET_NETIF_STATS,
	    (sysarg_t) device_id, NULL);
	async_data_read_start(exch, stats, sizeof(*stats));
	async_exchange_end(exch);
	
	sysarg_t result;
	async_wait_for(message_id, &result);
	
	return (int) result;
}

/** Create bidirectional connection with the network interface module
 *
 * Create bidirectional connection with the network interface module and
 * register the message receiver.
 *
 * @param[in] service   Network interface module service.
 * @param[in] device_id Device identifier.
 * @param[in] me        Requesting module service.
 * @param[in] receiver  Message receiver.
 *
 * @return Session to the needed service.
 * @return NULL on failure.
 *
 */
async_sess_t *netif_bind_service(services_t service, device_id_t device_id,
    services_t me, async_client_conn_t receiver)
{
	return bind_service(service, device_id, me, 0, receiver);
}

/** @}
 */
