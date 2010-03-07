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

/** @addtogroup net
 *  @{
 */

/** @file
 *  Networking common message definitions.
 */

#ifndef __NET_MESSAGES_H__
#define __NET_MESSAGES_H__

#include <async.h>

#include <ipc/ipc.h>
#include <ipc/services.h>

#include "include/device.h"

#include "structures/measured_strings.h"
#include "structures/packet/packet.h"

/** @name Networking message counts
 */
/*@{*/

/** The number of network interface driver messages.
 */
#define NET_NETIF_COUNT		6

/** The number of general networking messages.
 */
#define NET_NET_COUNT		3

/** The number of network interface layer messages.
 */
#define NET_NIL_COUNT		7

/** The number of Ethernet messages.
 */
#define NET_ETH_COUNT		0

/** The number of inter-network messages.
 */
#define NET_IL_COUNT		6

/** The number of IP messages.
 */
#define NET_IP_COUNT		4

/** The number of ARP messages.
 */
#define NET_ARP_COUNT		5

/** The number of ICMP messages.
 */
#define NET_ICMP_COUNT		6

/** The number of transport layer messages.
 */
#define NET_TL_COUNT		1

/** The number of UDP messages.
 */
#define NET_UDP_COUNT		0

/** The number of TCP messages.
 */
#define NET_TCP_COUNT		0

/** The number of packet management system messages.
 */
#define NET_PACKET_COUNT	5

/** The number of socket messages.
 */
#define NET_SOCKET_COUNT	14

/*@}*/

/** @name Networking message intervals
 */
/*@{*/

/** The first networking message.
 */
#define NET_FIRST			2000

/** The first network interface layer message.
 */
#define NET_NETIF_FIRST		NET_FIRST

/** The last network interface layer message.
 */
#define NET_NETIF_LAST		(NET_NETIF_FIRST + NET_NETIF_COUNT)

/** The first general networking message.
 */
#define NET_NET_FIRST		(NET_NETIF_LAST + 0)

/** The last general networking message.
 */
#define NET_NET_LAST		(NET_NET_FIRST + NET_NET_COUNT)

/** The first network interface layer message.
 */
#define NET_NIL_FIRST		(NET_NET_LAST + 0)

/** The last network interface layer message.
 */
#define NET_NIL_LAST		(NET_NIL_FIRST + NET_NIL_COUNT)

/** The first Ethernet message.
 */
#define NET_ETH_FIRST		(NET_NIL_LAST + 0)

/** The last Ethernet message.
 */
#define NET_ETH_LAST		(NET_ETH_FIRST + NET_ETH_COUNT)

/** The first inter-network message.
 */
#define NET_IL_FIRST		(NET_ETH_LAST + 0)

/** The last inter-network message.
 */
#define NET_IL_LAST			(NET_IL_FIRST + NET_IL_COUNT)

/** The first IP message.
 */
#define NET_IP_FIRST		(NET_IL_LAST + 0)

/** The last IP message.
 */
#define NET_IP_LAST			(NET_IP_FIRST + NET_IP_COUNT)

/** The first ARP message.
 */
#define NET_ARP_FIRST		(NET_IP_LAST + 0)

/** The last ARP message.
 */
#define NET_ARP_LAST		(NET_ARP_FIRST + NET_ARP_COUNT)

/** The first ICMP message.
 */
#define NET_ICMP_FIRST		(NET_ARP_LAST + 0)

/** The last ICMP message.
 */
#define NET_ICMP_LAST		(NET_ICMP_FIRST + NET_ICMP_COUNT)

/** The first ICMP message.
 */
#define NET_TL_FIRST		(NET_ICMP_LAST + 0)

/** The last ICMP message.
 */
#define NET_TL_LAST			(NET_TL_FIRST + NET_TL_COUNT)

/** The first UDP message.
 */
#define NET_UDP_FIRST		(NET_TL_LAST + 0)

/** The last UDP message.
 */
#define NET_UDP_LAST		(NET_UDP_FIRST + NET_UDP_COUNT)

/** The first TCP message.
 */
#define NET_TCP_FIRST		(NET_UDP_LAST + 0)

/** The last TCP message.
 */
#define NET_TCP_LAST		(NET_TCP_FIRST + NET_TCP_COUNT)

/** The first socket message.
 */
#define NET_SOCKET_FIRST	(NET_TCP_LAST + 0)

/** The last socket message.
 */
#define NET_SOCKET_LAST		(NET_SOCKET_FIRST + NET_SOCKET_COUNT)

/** The first packet management system message.
 */
#define NET_PACKET_FIRST	(NET_SOCKET_LAST + 0)

/** The last packet management system message.
 */
#define NET_PACKET_LAST		(NET_PACKET_FIRST + NET_PACKET_COUNT)

/** The last networking message.
 */
#define NET_LAST			NET_PACKET_LAST

/** The number of networking messages.
 */
#define NET_COUNT			(NET_LAST - NET_FIRST)

/** Returns a value indicating whether the value is in the interval.
 *  @param[in] item The value to be checked.
 *  @param[in] first_inclusive The first value in the interval inclusive.
 *  @param[in] last_exclusive The first value after the interval.
 */
#define IS_IN_INTERVAL(item, first_inclusive, last_exclusive)	(((item) >= (first_inclusive)) && ((item) < (last_exclusive)))

/** Returns a value indicating whether the IPC call is a generic networking message.
 *  @param[in] call The IPC call to be checked.
 */
#define IS_NET_MESSAGE(call)			IS_IN_INTERVAL(IPC_GET_METHOD(*call), NET_FIRST, NET_LAST)

/** Returns a value indicating whether the IPC call is a generic networking message.
 *  @param[in] call The IPC call to be checked.
 */
#define IS_NET_NET_MESSAGE(call)		IS_IN_INTERVAL(IPC_GET_METHOD(*call), NET_NET_FIRST, NET_NET_LAST)

/** Returns a value indicating whether the IPC call is a network interface layer message.
 *  @param[in] call The IPC call to be checked.
 */
#define IS_NET_NIL_MESSAGE(call)		IS_IN_INTERVAL(IPC_GET_METHOD(*call), NET_NIL_FIRST, NET_NIL_LAST)

/** Returns a value indicating whether the IPC call is an Ethernet message.
 *  @param[in] call The IPC call to be checked.
 */
#define IS_NET_ETH_MESSAGE(call)		IS_IN_INTERVAL(IPC_GET_METHOD(*call), NET_ETH_FIRST, NET_ETH_LAST)

/** Returns a value indicating whether the IPC call is an inter-network layer message.
 *  @param[in] call The IPC call to be checked.
 */
#define IS_NET_IL_MESSAGE(call)		IS_IN_INTERVAL(IPC_GET_METHOD(*call), NET_IL_FIRST, NET_IL_LAST)

/** Returns a value indicating whether the IPC call is an IP message.
 *  @param[in] call The IPC call to be checked.
 */
#define IS_NET_IP_MESSAGE(call)		IS_IN_INTERVAL(IPC_GET_METHOD(*call), NET_IP_FIRST, NET_IP_LAST)

/** Returns a value indicating whether the IPC call is an ARP message.
 *  @param[in] call The IPC call to be checked.
 */
#define IS_NET_ARP_MESSAGE(call)		IS_IN_INTERVAL(IPC_GET_METHOD(*call), NET_ARP_FIRST, NET_ARP_LAST)

/** Returns a value indicating whether the IPC call is an ICMP message.
 *  @param[in] call The IPC call to be checked.
 */
#define IS_NET_ICMP_MESSAGE(call)		IS_IN_INTERVAL(IPC_GET_METHOD(*call), NET_ICMP_FIRST, NET_ICMP_LAST)

/** Returns a value indicating whether the IPC call is a transport layer message.
 *  @param[in] call The IPC call to be checked.
 */
#define IS_NET_TL_MESSAGE(call)		IS_IN_INTERVAL(IPC_GET_METHOD(*call), NET_TL_FIRST, NET_TL_LAST)

/** Returns a value indicating whether the IPC call is a UDP message.
 *  @param[in] call The IPC call to be checked.
 */
#define IS_NET_UDP_MESSAGE(call)		IS_IN_INTERVAL(IPC_GET_METHOD(*call), NET_UDP_FIRST, NET_UDP_LAST)

/** Returns a value indicating whether the IPC call is a TCP message.
 *  @param[in] call The IPC call to be checked.
 */
#define IS_NET_TCP_MESSAGE(call)		IS_IN_INTERVAL(IPC_GET_METHOD(*call), NET_TCP_FIRST, NET_TCP_LAST)

/** Returns a value indicating whether the IPC call is a socket message.
 *  @param[in] call The IPC call to be checked.
 */
#define IS_NET_SOCKET_MESSAGE(call)	IS_IN_INTERVAL(IPC_GET_METHOD(*call), NET_SOCKET_FIRST, NET_SOCKET_LAST)

/** Returns a value indicating whether the IPC call is a packet manaagement system message.
 *  @param[in] call The IPC call to be checked.
 */
#define IS_NET_PACKET_MESSAGE(call)	IS_IN_INTERVAL(IPC_GET_METHOD(*call), NET_PACKET_FIRST, NET_PACKET_LAST)

/*@}*/

/** @name Networking specific message parameters definitions
 */
/*@{*/

/** Returns the device identifier message parameter.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_DEVICE(call)		(device_id_t) IPC_GET_ARG1(*call)

/** Returns the packet identifier message parameter.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_PACKET(call)		(packet_id_t) IPC_GET_ARG2(*call)

/** Returns the count message parameter.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_COUNT(call)		(size_t) IPC_GET_ARG2(*call)

/** Returns the device state message parameter.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_STATE(call)		(device_state_t) IPC_GET_ARG2(*call)

/** Returns the maximum transmission unit message parameter.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_MTU(call)			(size_t) IPC_GET_ARG2(*call)

/** Returns the device driver service message parameter.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_SERVICE(call)		(services_t) IPC_GET_ARG3(*call)

/** Returns the target service message parameter.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_TARGET(call)		(services_t) IPC_GET_ARG3(*call)

/** Returns the sender service message parameter.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_SENDER(call)		(services_t) IPC_GET_ARG3(*call)

/** Returns the error service message parameter.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_ERROR(call)		(services_t) IPC_GET_ARG4(*call)

/** Returns the phone message parameter.
 *  @param[in] call The message call structure.
 */
#define IPC_GET_PHONE(call)		(int) IPC_GET_ARG5(*call)

/** Sets the device identifier in the message answer.
 *  @param[out] answer The message answer structure.
 */
#define IPC_SET_DEVICE(answer)	((device_id_t *) &IPC_GET_ARG1(*answer))

/** Sets the minimum address length in the message answer.
 *  @param[out] answer The message answer structure.
 */
#define IPC_SET_ADDR(answer)		((size_t *) &IPC_GET_ARG1(*answer))

/** Sets the minimum prefix size in the message answer.
 *  @param[out] answer The message answer structure.
 */
#define IPC_SET_PREFIX(answer)	((size_t *) &IPC_GET_ARG2(*answer))

/** Sets the maximum content size in the message answer.
 *  @param[out] answer The message answer structure.
 */
#define IPC_SET_CONTENT(answer)	((size_t *) &IPC_GET_ARG3(*answer))

/** Sets the minimum suffix size in the message answer.
 *  @param[out] answer The message answer structure.
 */
#define IPC_SET_SUFFIX(answer)	((size_t *) &IPC_GET_ARG4(*answer))

/*@}*/

/** Returns the address.
 *  @param[in] phone The service module phone.
 *  @param[in] message The service specific message.
 *  @param[in] device_id The device identifier.
 *  @param[out] address The desired address.
 *  @param[out] data The address data container.
 *  @returns EOK on success.
 *  @returns EBADMEM if the address parameter and/or the data parameter is NULL.
 *  @returns Other error codes as defined for the specific service message.
 */
static inline int generic_get_addr_req(int phone, int message, device_id_t device_id, measured_string_ref * address, char ** data){
	aid_t message_id;
	ipcarg_t result;
	int string;

	if(!(address && data)){
		return EBADMEM;
	}
	message_id = async_send_1(phone, (ipcarg_t) message, (ipcarg_t) device_id, NULL);
	string = measured_strings_return(phone, address, data, 1);
	async_wait_for(message_id, &result);
	if((string == EOK) && (result != EOK)){
		free(*address);
		free(*data);
	}
	return (int) result;
}

/** Translates the given strings.
 *  Allocates and returns the needed memory block as the data parameter.
 *  @param[in] phone The service module phone.
 *  @param[in] message The service specific message.
 *  @param[in] device_id The device identifier.
 *  @param[in] service The module service.
 *  @param[in] configuration The key strings.
 *  @param[in] count The number of configuration keys.
 *  @param[out] translation The translated values.
 *  @param[out] data The translation data container.
 *  @returns EOK on success.
 *  @returns EINVAL if the configuration parameter is NULL.
 *  @returns EINVAL if the count parameter is zero (0).
 *  @returns EBADMEM if the translation or the data parameters are NULL.
 *  @returns Other error codes as defined for the specific service message.
 */
static inline int generic_translate_req(int phone, int message, device_id_t device_id, services_t service, measured_string_ref configuration, size_t count, measured_string_ref * translation, char ** data){
	aid_t message_id;
	ipcarg_t result;
	int string;

	if(!(configuration && (count > 0))){
		return EINVAL;
	}
	if(!(translation && data)){
		return EBADMEM;
	}
	message_id = async_send_3(phone, (ipcarg_t) message, (ipcarg_t) device_id, (ipcarg_t) count, (ipcarg_t) service, NULL);
	measured_strings_send(phone, configuration, count);
	string = measured_strings_return(phone, translation, data, count);
	async_wait_for(message_id, &result);
	if((string == EOK) && (result != EOK)){
		free(*translation);
		free(*data);
	}
	return (int) result;
}

/** Sends the packet queue.
 *  @param[in] phone The service module phone.
 *  @param[in] message The service specific message.
 *  @param[in] device_id The device identifier.
 *  @param[in] packet_id The packet or the packet queue identifier.
 *  @param[in] sender The sending module service.
 *  @param[in] error The error module service.
 *  @returns EOK on success.
 */
static inline int generic_send_msg(int phone, int message, device_id_t device_id, packet_id_t packet_id, services_t sender, services_t error){
	if(error){
		async_msg_4(phone, (ipcarg_t) message, (ipcarg_t) device_id, (ipcarg_t) packet_id, (ipcarg_t) sender, (ipcarg_t) error);
	}else{
		async_msg_3(phone, (ipcarg_t) message, (ipcarg_t) device_id, (ipcarg_t) packet_id, (ipcarg_t) sender);
	}
	return EOK;
}

/** Returns the device packet dimension for sending.
 *  @param[in] phone The service module phone.
 *  @param[in] message The service specific message.
 *  @param[in] device_id The device identifier.
 *  @param[out] packet_dimension The packet dimension.
 *  @returns EOK on success.
 *  @returns EBADMEM if the packet_dimension parameter is NULL.
 *  @returns Other error codes as defined for the specific service message.
 */
static inline int generic_packet_size_req(int phone, int message, device_id_t device_id, packet_dimension_ref packet_dimension){
	ipcarg_t result;
	ipcarg_t prefix;
	ipcarg_t content;
	ipcarg_t suffix;
	ipcarg_t addr_len;

	if(! packet_dimension){
		return EBADMEM;
	}
	result = async_req_1_4(phone, (ipcarg_t) message, (ipcarg_t) device_id, &addr_len, &prefix, &content, &suffix);
	packet_dimension->prefix = (size_t) prefix;
	packet_dimension->content = (size_t) content;
	packet_dimension->suffix = (size_t) suffix;
	packet_dimension->addr_len = (size_t) addr_len;
	return (int) result;
}

/** Notifies the module about the device state change.
 *  @param[in] phone The service module phone.
 *  @param[in] message The service specific message.
 *  @param[in] device_id The device identifier.
 *  @param[in] state The new device state.
 *  @param[in] target The target module service.
 *  @returns EOK on success.
 */
static inline int generic_device_state_msg(int phone, int message, device_id_t device_id, int state, services_t target){
	async_msg_3(phone, (ipcarg_t) message, (ipcarg_t) device_id, (ipcarg_t) state, target);
	return EOK;
}

/** Passes the packet queue to the module.
 *  @param[in] phone The service module phone.
 *  @param[in] message The service specific message.
 *  @param[in] device_id The device identifier.
 *  @param[in] packet_id The received packet or the received packet queue identifier.
 *  @param[in] target The target module service.
 *  @param[in] error The error module service.
 *  @returns EOK on success.
 */
static inline int generic_received_msg(int phone, int message, device_id_t device_id, packet_id_t packet_id, services_t target, services_t error){
	if(error){
		async_msg_4(phone, (ipcarg_t) message, (ipcarg_t) device_id, (ipcarg_t) packet_id, (ipcarg_t) target, (ipcarg_t) error);
	}else{
		async_msg_3(phone, (ipcarg_t) message, (ipcarg_t) device_id, (ipcarg_t) packet_id, (ipcarg_t) target);
	}
	return EOK;
}

/** Notifies a module about the device.
 *  @param[in] phone The service module phone.
 *  @param[in] message The service specific message.
 *  @param[in] device_id The device identifier.
 *  @param[in] arg2 The second argument of the message.
 *  @param[in] service The device module service.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the specific service message.
 */
static inline int generic_device_req(int phone, int message, device_id_t device_id, int arg2, services_t service){
	return (int) async_req_3_0(phone, (ipcarg_t) message, (ipcarg_t) device_id, (ipcarg_t) arg2, (ipcarg_t) service);
}

#endif

/** @}
 */
