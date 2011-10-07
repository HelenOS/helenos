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
 *
 * IP interface implementation for remote modules.
 *
 * @see ip_interface.h
 * @see il_remote.h
 *
 */

#include <ip_remote.h>
#include <ip_interface.h>
#include <packet_client.h>
#include <generic.h>
#include <ipc/services.h>
#include <ipc/il.h>
#include <ipc/ip.h>
#include <net/modules.h>
#include <net/device.h>
#include <net/inet.h>

/** Add a route to the device routing table.
 *
 * The target network is routed using this device.
 *
 * @param[in] sess      IP module sessions.
 * @param[in] device_id Device identifier.
 * @param[in] address   Target network address.
 * @param[in] netmask   Target network mask.
 * @param[in] gateway   Target network gateway. Not used if zero.
 *
 */
int ip_add_route_req_remote(async_sess_t *sess, nic_device_id_t device_id,
    in_addr_t address, in_addr_t netmask, in_addr_t gateway)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_4_0(exch, NET_IP_ADD_ROUTE,
	    (sysarg_t) device_id, (sysarg_t) gateway.s_addr,
	    (sysarg_t) address.s_addr, (sysarg_t) netmask.s_addr);
	async_exchange_end(exch);
	
	return rc;
}

/** Create bidirectional connection with the ip module service and register
 * the message receiver.
 *
 * @param[in] service  IP module service.
 * @param[in] protocol Transport layer protocol.
 * @param[in] me       Rquesting module service.
 * @param[in] receiver Message receiver. Used for remote connection.
 *
 * @return Session to the needed service.
 * @return NULL on failure.
 *
 */
async_sess_t *ip_bind_service(services_t service, int protocol, services_t me,
    async_client_conn_t receiver)
{
	return bind_service(service, (sysarg_t) protocol, me, service,
	    receiver);
}

/** Connect to the IP module.
 *
 * @return The IP module session.
 *
 */
async_sess_t *ip_connect_module(services_t service)
{
	// FIXME: Get rid of the useless argument
	return connect_to_service(SERVICE_IP);
}

/** Register the new device.
 *
 * Register itself as the ip packet receiver.
 * If the device uses ARP registers also the new ARP device.
 *
 * @param[in] sess      IP module session.
 * @param[in] device_id New device identifier.
 *
 * @return EOK on success.
 * @return ENOMEM if there is not enough memory left.
 * @return EINVAL if the device configuration is invalid.
 * @return ENOTSUP if the device uses IPv6.
 * @return ENOTSUP if the device uses DHCP.
 * @return Other error codes as defined for the
 *         net_get_device_conf_req() function.
 * @return Other error codes as defined for the arp_device_req()
 *         function.
 *
 */
int ip_device_req_remote(async_sess_t *sess, nic_device_id_t device_id,
    services_t service)
{
	return generic_device_req_remote(sess, NET_IP_DEVICE, device_id,
	    service);
}

/** Return the device identifier and the IP pseudo header based on the
 * destination address.
 *
 * @param[in] sess        IP module session.
 * @param[in] protocol    Transport protocol.
 * @param[in] destination Destination address.
 * @param[in] addrlen     Destination address length.
 * @param[out] device_id  Device identifier.
 * @param[out] header     Constructed IP pseudo header.
 * @param[out] headerlen  IP pseudo header length.
 *
 */
int ip_get_route_req_remote(async_sess_t *sess, ip_protocol_t protocol,
    const struct sockaddr *destination, socklen_t addrlen,
    nic_device_id_t *device_id, void **header, size_t *headerlen)
{
	if ((!destination) || (addrlen == 0))
		return EINVAL;
	
	if ((!device_id) || (!header) || (!headerlen))
		return EBADMEM;
	
	*header = NULL;
	
	async_exch_t *exch = async_exchange_begin(sess);
	
	ipc_call_t answer;
	aid_t message_id = async_send_1(exch, NET_IP_GET_ROUTE,
	    (sysarg_t) protocol, &answer);
	
	if ((async_data_write_start(exch, destination, addrlen) == EOK) &&
	    (async_data_read_start(exch, headerlen, sizeof(*headerlen)) == EOK) &&
	    (*headerlen > 0)) {
		*header = malloc(*headerlen);
		if (*header) {
			if (async_data_read_start(exch, *header,
			    *headerlen) != EOK)
				free(*header);
		}
	}
	
	async_exchange_end(exch);
	
	sysarg_t result;
	async_wait_for(message_id, &result);
	
	if ((result != EOK) && *header)
		free(*header);
	else
		*device_id = IPC_GET_DEVICE(answer);
	
	return (int) result;
}

/** Return the device packet dimension for sending.
 *
 * @param[in] sess              IP module session.
 * @param[in] device_id         Device identifier.
 * @param[out] packet_dimension Packet dimension.
 *
 * @return EOK on success.
 * @return ENOENT if there is no such device.
 * @return Other error codes as defined for the
 *         generic_packet_size_req_remote() function.
 *
 */
int ip_packet_size_req_remote(async_sess_t *sess, nic_device_id_t device_id,
    packet_dimension_t *packet_dimension)
{
	return generic_packet_size_req_remote(sess, NET_IP_PACKET_SPACE,
	    device_id, packet_dimension);
}

/** Notify the IP module about the received error notification packet.
 *
 * @param[in] sess      IP module session.
 * @param[in] device_id Device identifier.
 * @param[in] packet    Received packet or the received packet queue.
 * @param[in] target    Target internetwork module service to be
 *                      delivered to.
 * @param[in] error     Packet error reporting service. Prefixes the
 *                      received packet.
 *
 * @return EOK on success.
 *
 */
int ip_received_error_msg_remote(async_sess_t *sess, nic_device_id_t device_id,
    packet_t *packet, services_t target, services_t error)
{
	return generic_received_msg_remote(sess, NET_IP_RECEIVED_ERROR,
	    device_id, packet_get_id(packet), target, error);
}

/** Send the packet queue.
 *
 * The packets may get fragmented if needed.
 *
 * @param[in] sess      IP module session.
 * @param[in] device_id Device identifier.
 * @param[in] packet    Packet fragments as a packet queue. All the
 *                      packets have to have the same destination address.
 * @param[in] sender    Sending module service.
 * @param[in] error     Packet error reporting service. Prefixes the
 *                      received packet.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the generic_send_msg()
 *         function.
 *
 */
int ip_send_msg_remote(async_sess_t *sess, nic_device_id_t device_id,
    packet_t *packet, services_t sender, services_t error)
{
	return generic_send_msg_remote(sess, NET_IP_SEND, device_id,
	    packet_get_id(packet), sender, error);
}

/** Set the default gateway.
 *
 * This gateway is used if no other route is found.
 *
 * @param[in] sess      IP module session.
 * @param[in] device_id Device identifier.
 * @param[in] gateway   Default gateway.
 *
 */
int ip_set_gateway_req_remote(async_sess_t *sess, nic_device_id_t device_id,
    in_addr_t gateway)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_2_0(exch, NET_IP_SET_GATEWAY,
	    (sysarg_t) device_id, (sysarg_t) gateway.s_addr);
	async_exchange_end(exch);
	
	return rc;
}

/** @}
 */
