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

/** @addtogroup ip
 * @{
 */

/** @file
 *
 * IP interface implementation for remote modules.
 *
 * @see ip_interface.h
 * @see il_interface.h
 *
 */

#include <ipc/services.h>

#include <net_messages.h>
#include <net_modules.h>
#include <net_device.h>
#include <inet.h>
#include <ip_interface.h>
#include <packet/packet_client.h>
#include <il_messages.h>
#include <ip_messages.h>
#include <ip_remote.h>

/** Add a route to the device routing table.
 *
 * The target network is routed using this device.
 *
 * @param[in] ip_phone  The IP module phone used for (semi)remote calls.
 * @param[in] device_id The device identifier.
 * @param[in] address   The target network address.
 * @param[in] netmask   The target network mask.
 * @param[in] gateway   The target network gateway. Not used if zero.
 *
 */
int ip_add_route_req_remote(int ip_phone, device_id_t device_id,
    in_addr_t address, in_addr_t netmask, in_addr_t gateway)
{
	return (int) async_req_4_0(ip_phone, NET_IP_ADD_ROUTE,
	    (ipcarg_t) device_id, (ipcarg_t) gateway.s_addr,
	    (ipcarg_t) address.s_addr, (ipcarg_t) netmask.s_addr);
}

int ip_bind_service(services_t service, int protocol, services_t me,
    async_client_conn_t receiver, tl_received_msg_t tl_received_msg)
{
	return (int) bind_service(service, (ipcarg_t) protocol, me, service,
	    receiver);
}

int ip_connect_module(services_t service)
{
	return connect_to_service(SERVICE_IP);
}

/** Register the new device.
 *
 * Register itself as the ip packet receiver.
 * If the device uses ARP registers also the new ARP device.
 *
 * @param[in] ip_phone  The IP module phone used for (semi)remote calls.
 * @param[in] device_id The new device identifier.
 * @param[in] netif     The underlying device network interface layer service.
 *
 * @return EOK on success.
 * @return ENOMEM if there is not enough memory left.
 * @return EINVAL if the device configuration is invalid.
 * @return ENOTSUP if the device uses IPv6.
 * @return ENOTSUP if the device uses DHCP.
 * @return Other error codes as defined for the net_get_device_conf_req()
 *         function.
 * @return Other error codes as defined for the arp_device_req() function.
 *
 */
int ip_device_req_remote(int ip_phone, device_id_t device_id,
    services_t service)
{
	return generic_device_req_remote(ip_phone, NET_IL_DEVICE, device_id, 0,
	    service);
}

/** Return the device identifier and the IP pseudo header based on the destination address.
 *
 * @param[in]  ip_phone    The IP module phone used for (semi)remote calls.
 * @param[in]  protocol    The transport protocol.
 * @param[in]  destination The destination address.
 * @param[in]  addrlen     The destination address length.
 * @param[out] device_id   The device identifier.
 * @param[out] header      The constructed IP pseudo header.
 * @param[out] headerlen   The IP pseudo header length.
 *
 */
int ip_get_route_req_remote(int ip_phone, ip_protocol_t protocol,
    const struct sockaddr *destination, socklen_t addrlen,
    device_id_t *device_id, void **header, size_t *headerlen)
{
	if ((!destination) || (addrlen == 0))
		return EINVAL;
	
	if ((!device_id) || (header) || (headerlen))
		return EBADMEM;
	
	*header = NULL;
	
	ipc_call_t answer;
	aid_t message_id = async_send_1(ip_phone, NET_IP_GET_ROUTE,
	    (ipcarg_t) protocol, &answer);
	
	if ((async_data_write_start(ip_phone, destination, addrlen) == EOK)
	    && (async_data_read_start(ip_phone, headerlen, sizeof(*headerlen)) == EOK)
	    && (*headerlen > 0)) {
		*header = malloc(*headerlen);
		if (*header) {
			if (async_data_read_start(ip_phone, *header, *headerlen) != EOK)
				free(*header);
		}
	}
	
	ipcarg_t result;
	async_wait_for(message_id, &result);
	
	if ((result != EOK) && (*header))
		free(*header);
	else
		*device_id = IPC_GET_DEVICE(&answer);
	
	return (int) result;
}

/** Return the device packet dimension for sending.
 *
 * @param[in]  ip_phone         The IP module phone used for (semi)remote calls.
 * @param[in]  device_id        The device identifier.
 * @param[out] packet_dimension The packet dimension.
 *
 * @return EOK on success.
 * @return ENOENT if there is no such device.
 * @return Other error codes as defined for the
 *         generic_packet_size_req_remote() function.
 *
 */
int ip_packet_size_req_remote(int ip_phone, device_id_t device_id,
    packet_dimension_ref packet_dimension)
{
	return generic_packet_size_req_remote(ip_phone, NET_IL_PACKET_SPACE, device_id,
	    packet_dimension);
}

/** Notify the IP module about the received error notification packet.
 *
 * @param[in] ip_phone  The IP module phone used for (semi)remote calls.
 * @param[in] device_id The device identifier.
 * @param[in] packet    The received packet or the received packet queue.
 * @param[in] target    The target internetwork module service to be
 *                      delivered to.
 * @param[in] error     The packet error reporting service. Prefixes the
 *                      received packet.
 *
 * @return EOK on success.
 *
 */
int ip_received_error_msg_remote(int ip_phone, device_id_t device_id,
    packet_t packet, services_t target, services_t error)
{
	return generic_received_msg_remote(ip_phone, NET_IP_RECEIVED_ERROR,
	    device_id, packet_get_id(packet), target, error);
}

/** Send the packet queue.
 *
 * The packets may get fragmented if needed.
 *
 * @param[in] ip_phone  The IP module phone used for (semi)remote calls.
 * @param[in] device_id The device identifier.
 * @param[in] packet    The packet fragments as a packet queue. All the
 *                      packets have to have the same destination address.
 * @param[in] sender    The sending module service.
 * @param[in] error     The packet error reporting service. Prefixes the
 *                      received packet.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the generic_send_msg() function.
 *
 */
int ip_send_msg_remote(int ip_phone, device_id_t device_id, packet_t packet,
    services_t sender, services_t error)
{
	return generic_send_msg_remote(ip_phone, NET_IL_SEND, device_id,
	    packet_get_id(packet), sender, error);
}

/** Set the default gateway.
 *
 * This gateway is used if no other route is found.
 *
 * @param[in] ip_phone  The IP module phone used for (semi)remote calls.
 * @param[in] device_id The device identifier.
 * @param[in] gateway   The default gateway.
 *
 */
int ip_set_gateway_req_remote(int ip_phone, device_id_t device_id,
    in_addr_t gateway)
{
	return (int) async_req_2_0(ip_phone, NET_IP_SET_GATEWAY,
	    (ipcarg_t) device_id, (ipcarg_t) gateway.s_addr);
}

/** @}
 */
