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

/** @addtogroup libcipc
 *  @{
 */

/** @file
 *  Networking common message definitions.
 */

#ifndef LIBC_NET_MESSAGES_H_
#define LIBC_NET_MESSAGES_H_

#include <ipc/ipc.h>
#include <ipc/services.h>

#include <net/device.h>
#include <net/packet.h>

/** Returns a value indicating whether the value is in the interval.
 * @param[in] item	The value to be checked.
 * @param[in] first_inclusive The first value in the interval inclusive.
 * @param[in] last_exclusive The first value after the interval.
 */
#define IS_IN_INTERVAL(item, first_inclusive, last_exclusive) \
	(((item) >= (first_inclusive)) && ((item) < (last_exclusive)))

/** @name Networking message counts */
/*@{*/

/** The number of ARP messages. */
#define NET_ARP_COUNT		5

/** The number of Ethernet messages. */
#define NET_ETH_COUNT		0

/** The number of ICMP messages. */
#define NET_ICMP_COUNT		6

/** The number of inter-network messages. */
#define NET_IL_COUNT		6

/** The number of IP messages. */
#define NET_IP_COUNT		4

/** The number of general networking messages. */
#define NET_NET_COUNT		3

/** The number of network interface driver messages. */
#define NET_NETIF_COUNT		6

/** The number of network interface layer messages. */
#define NET_NIL_COUNT		7

/** The number of packet management system messages. */
#define NET_PACKET_COUNT	5

/** The number of socket messages. */
#define NET_SOCKET_COUNT	14

/** The number of TCP messages. */
#define NET_TCP_COUNT		0

/** The number of transport layer messages. */
#define NET_TL_COUNT		1

/** The number of UDP messages. */
#define NET_UDP_COUNT		0

/*@}*/

/** @name Networking message intervals
 */
/*@{*/

/** The first networking message. */
#define NET_FIRST		2000

/** The first network interface layer message. */
#define NET_NETIF_FIRST		NET_FIRST

/** The last network interface layer message. */
#define NET_NETIF_LAST		(NET_NETIF_FIRST + NET_NETIF_COUNT)

/** The first general networking message. */
#define NET_NET_FIRST		(NET_NETIF_LAST + 0)

/** The last general networking message. */
#define NET_NET_LAST		(NET_NET_FIRST + NET_NET_COUNT)

/** The first network interface layer message. */
#define NET_NIL_FIRST		(NET_NET_LAST + 0)

/** The last network interface layer message. */
#define NET_NIL_LAST		(NET_NIL_FIRST + NET_NIL_COUNT)

/** The first Ethernet message. */
#define NET_ETH_FIRST		(NET_NIL_LAST + 0)

/** The last Ethernet message. */
#define NET_ETH_LAST		(NET_ETH_FIRST + NET_ETH_COUNT)

/** The first inter-network message. */
#define NET_IL_FIRST		(NET_ETH_LAST + 0)

/** The last inter-network message. */
#define NET_IL_LAST		(NET_IL_FIRST + NET_IL_COUNT)

/** The first IP message. */
#define NET_IP_FIRST		(NET_IL_LAST + 0)

/** The last IP message. */
#define NET_IP_LAST		(NET_IP_FIRST + NET_IP_COUNT)

/** The first ARP message. */
#define NET_ARP_FIRST		(NET_IP_LAST + 0)

/** The last ARP message. */
#define NET_ARP_LAST		(NET_ARP_FIRST + NET_ARP_COUNT)

/** The first ICMP message. */
#define NET_ICMP_FIRST		(NET_ARP_LAST + 0)

/** The last ICMP message. */
#define NET_ICMP_LAST		(NET_ICMP_FIRST + NET_ICMP_COUNT)

/** The first ICMP message. */
#define NET_TL_FIRST		(NET_ICMP_LAST + 0)

/** The last ICMP message. */
#define NET_TL_LAST		(NET_TL_FIRST + NET_TL_COUNT)

/** The first UDP message. */
#define NET_UDP_FIRST		(NET_TL_LAST + 0)

/** The last UDP message. */
#define NET_UDP_LAST		(NET_UDP_FIRST + NET_UDP_COUNT)

/** The first TCP message. */
#define NET_TCP_FIRST		(NET_UDP_LAST + 0)

/** The last TCP message. */
#define NET_TCP_LAST		(NET_TCP_FIRST + NET_TCP_COUNT)

/** The first socket message. */
#define NET_SOCKET_FIRST	(NET_TCP_LAST + 0)

/** The last socket message. */
#define NET_SOCKET_LAST		(NET_SOCKET_FIRST + NET_SOCKET_COUNT)

/** The first packet management system message. */
#define NET_PACKET_FIRST	(NET_SOCKET_LAST + 0)

/** The last packet management system message. */
#define NET_PACKET_LAST		(NET_PACKET_FIRST + NET_PACKET_COUNT)

/** The last networking message. */
#define NET_LAST		NET_PACKET_LAST

/** The number of networking messages. */
#define NET_COUNT		(NET_LAST - NET_FIRST)

/** Returns a value indicating whether the IPC call is a generic networking
 * message.
 * @param[in] call The IPC call to be checked.
 */
#define IS_NET_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(*call), NET_FIRST, NET_LAST)

/** Returns a value indicating whether the IPC call is an ARP message.
 * @param[in] call The IPC call to be checked.
 */
#define IS_NET_ARP_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(*call), NET_ARP_FIRST, NET_ARP_LAST)

/** Returns a value indicating whether the IPC call is an Ethernet message.
 * @param[in] call The IPC call to be checked.
 */
#define IS_NET_ETH_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(*call), NET_ETH_FIRST, NET_ETH_LAST)

/** Returns a value indicating whether the IPC call is an ICMP message.
 * @param[in] call The IPC call to be checked.
 */
#define IS_NET_ICMP_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(*call), NET_ICMP_FIRST, NET_ICMP_LAST)

/** Returns a value indicating whether the IPC call is an inter-network layer
 * message.
 * @param[in] call The IPC call to be checked.
 */
#define IS_NET_IL_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(*call), NET_IL_FIRST, NET_IL_LAST)

/** Returns a value indicating whether the IPC call is an IP message.
 * @param[in] call The IPC call to be checked.
 */
#define IS_NET_IP_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(*call), NET_IP_FIRST, NET_IP_LAST)

/** Returns a value indicating whether the IPC call is a generic networking
 * message.
 * @param[in] call The IPC call to be checked.
 */
#define IS_NET_NET_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(*call), NET_NET_FIRST, NET_NET_LAST)

/** Returns a value indicating whether the IPC call is a network interface layer
 * message.
 * @param[in] call The IPC call to be checked.
 */
#define IS_NET_NIL_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(*call), NET_NIL_FIRST, NET_NIL_LAST)

/** Returns a value indicating whether the IPC call is a packet manaagement
 * system message.
 * @param[in] call The IPC call to be checked.
 */
#define IS_NET_PACKET_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(*call), NET_PACKET_FIRST, NET_PACKET_LAST)

/** Returns a value indicating whether the IPC call is a socket message.
 * @param[in] call The IPC call to be checked.
 */
#define IS_NET_SOCKET_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(*call), NET_SOCKET_FIRST, NET_SOCKET_LAST)

/** Returns a value indicating whether the IPC call is a TCP message.
 * @param[in] call The IPC call to be checked.
 */
#define IS_NET_TCP_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(*call), NET_TCP_FIRST, NET_TCP_LAST)

/** Returns a value indicating whether the IPC call is a transport layer message.
 * @param[in] call The IPC call to be checked.
 */
#define IS_NET_TL_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(*call), NET_TL_FIRST, NET_TL_LAST)

/** Returns a value indicating whether the IPC call is a UDP message.
 * @param[in] call The IPC call to be checked.
 */
#define IS_NET_UDP_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(*call), NET_UDP_FIRST, NET_UDP_LAST)

/*@}*/

/** @name Networking specific message arguments definitions */
/*@{*/

/** Returns the device identifier message argument.
 * @param[in] call The message call structure.
 */
#define IPC_GET_DEVICE(call) \
	({ \
		device_id_t device_id = (device_id_t) IPC_GET_ARG1(*call); \
		device_id; \
	})

/** Returns the packet identifier message argument.
 * @param[in] call The message call structure.
 */
#define IPC_GET_PACKET(call) \
	({ \
		packet_id_t packet_id = (packet_id_t) IPC_GET_ARG2(*call); \
		packet_id; \
	})

/** Returns the count message argument.
 * @param[in] call The message call structure.
 */
#define IPC_GET_COUNT(call) \
	({ \
		size_t size = (size_t) IPC_GET_ARG2(*call); \
		size; \
	})

/** Returns the device state message argument.
 * @param[in] call The message call structure.
 */
#define IPC_GET_STATE(call) \
	({ \
		device_state_t state = (device_state_t) IPC_GET_ARG2(*call); \
		state; \
	})

/** Returns the maximum transmission unit message argument.
 * @param[in] call The message call structure.
 */
#define IPC_GET_MTU(call) \
	({ \
		size_t size = (size_t) IPC_GET_ARG2(*call); \
		size; \
	})

/** Returns the device driver service message argument.
 * @param[in] call The message call structure.
 */
#define IPC_GET_SERVICE(call) \
	({ \
		services_t service = (services_t) IPC_GET_ARG3(*call); \
		service; \
	})

/** Returns the target service message argument.
 * @param[in] call The message call structure.
 */
#define IPC_GET_TARGET(call) \
	({ \
		services_t service = (services_t) IPC_GET_ARG3(*call); \
		service; \
	})

/** Returns the sender service message argument.
 * @param[in] call The message call structure.
 */
#define IPC_GET_SENDER(call) \
	({ \
		services_t service = (services_t) IPC_GET_ARG3(*call); \
		service; \
	})

/** Returns the error service message argument.
 * @param[in] call The message call structure.
 */
#define IPC_GET_ERROR(call) \
	({ \
		services_t service = (services_t) IPC_GET_ARG4(*call); \
		service; \
	})

/** Returns the phone message argument.
 * @param[in] call The message call structure.
 */
#define IPC_GET_PHONE(call) \
	({ \
		int phone = (int) IPC_GET_ARG5(*call); \
		phone; \
	})

/** Sets the device identifier in the message answer.
 * @param[out] answer The message answer structure.
 */
#define IPC_SET_DEVICE(answer, value) \
	do { \
		sysarg_t argument = (sysarg_t) (value); \
		IPC_SET_ARG1(*answer, argument); \
	} while (0)

/** Sets the minimum address length in the message answer.
 * @param[out] answer The message answer structure.
 */
#define IPC_SET_ADDR(answer, value) \
	do { \
		sysarg_t argument = (sysarg_t) (value); \
		IPC_SET_ARG1(*answer, argument); \
	} while (0)

/** Sets the minimum prefix size in the message answer.
 * @param[out] answer The message answer structure.
 */
#define IPC_SET_PREFIX(answer, value) \
	do { \
		sysarg_t argument = (sysarg_t) (value); \
		IPC_SET_ARG2(*answer, argument); \
	} while (0)

/** Sets the maximum content size in the message answer.
 * @param[out] answer The message answer structure.
 */
#define IPC_SET_CONTENT(answer, value) \
	do { \
		sysarg_t argument = (sysarg_t) (value); \
		IPC_SET_ARG3(*answer, argument); \
	} while (0)

/** Sets the minimum suffix size in the message answer.
 * @param[out] answer The message answer structure.
 */
#define IPC_SET_SUFFIX(answer, value) \
	do { \
		sysarg_t argument = (sysarg_t) (value); \
		IPC_SET_ARG4(*answer, argument); \
	} while (0)

/*@}*/

#endif

/** @}
 */
