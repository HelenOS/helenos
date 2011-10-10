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

#include <ipc/services.h>
#include <net/device.h>
#include <net/packet.h>

/** Return a value indicating whether the value is in the interval.
 *
 * @param[in] item            Value to be checked.
 * @param[in] first_inclusive First value in the interval inclusive.
 * @param[in] last_exclusive  First value after the interval.
 *
 */
#define IS_IN_INTERVAL(item, first_inclusive, last_exclusive) \
	(((item) >= (first_inclusive)) && ((item) < (last_exclusive)))

/** @name Networking message counts */
/*@{*/

#define NET_ARP_COUNT     5   /**< Number of ARP messages. */
#define NET_ETH_COUNT     0   /**< Number of Ethernet messages. */
#define NET_ICMP_COUNT    6   /**< Number of ICMP messages. */
#define NET_IL_COUNT      6   /**< Number of inter-network messages. */
#define NET_IP_COUNT      4   /**< Number of IP messages. */
#define NET_NET_COUNT     3   /**< Number of general networking messages. */
#define NET_NETIF_COUNT   6   /**< Number of network interface driver messages. */
#define NET_NIL_COUNT     7   /**< Number of network interface layer messages. */
#define NET_PACKET_COUNT  5   /**< Number of packet management system messages. */
#define NET_SOCKET_COUNT  14  /**< Number of socket messages. */
#define NET_TCP_COUNT     0   /**< Number of TCP messages. */
#define NET_TL_COUNT      1   /**< Number of transport layer messages. */
#define NET_UDP_COUNT     0   /**< Number of UDP messages. */

/*@}*/

/** @name Networking message intervals
 */
/*@{*/


/** First networking message. */
#define NET_FIRST  2000

/** First network interface layer message. */
#define NET_NETIF_FIRST  NET_FIRST

/** Last network interface layer message. */
#define NET_NETIF_LAST  (NET_NETIF_FIRST + NET_NETIF_COUNT)

/** First general networking message. */
#define NET_NET_FIRST  (NET_NETIF_LAST + 0)

/** Last general networking message. */
#define NET_NET_LAST  (NET_NET_FIRST + NET_NET_COUNT)

/** First network interface layer message. */
#define NET_NIL_FIRST  (NET_NET_LAST + 0)

/** Last network interface layer message. */
#define NET_NIL_LAST  (NET_NIL_FIRST + NET_NIL_COUNT)

/** First Ethernet message. */
#define NET_ETH_FIRST  (NET_NIL_LAST + 0)

/** Last Ethernet message. */
#define NET_ETH_LAST  (NET_ETH_FIRST + NET_ETH_COUNT)

/** First inter-network message. */
#define NET_IL_FIRST  (NET_ETH_LAST + 0)

/** Last inter-network message. */
#define NET_IL_LAST  (NET_IL_FIRST + NET_IL_COUNT)

/** First IP message. */
#define NET_IP_FIRST  (NET_IL_LAST + 0)

/** Last IP message. */
#define NET_IP_LAST  (NET_IP_FIRST + NET_IP_COUNT)

/** First ARP message. */
#define NET_ARP_FIRST  (NET_IP_LAST + 0)

/** Last ARP message. */
#define NET_ARP_LAST  (NET_ARP_FIRST + NET_ARP_COUNT)

/** First ICMP message. */
#define NET_ICMP_FIRST  (NET_ARP_LAST + 0)

/** Last ICMP message. */
#define NET_ICMP_LAST  (NET_ICMP_FIRST + NET_ICMP_COUNT)

/** First ICMP message. */
#define NET_TL_FIRST  (NET_ICMP_LAST + 0)

/** Last ICMP message. */
#define NET_TL_LAST  (NET_TL_FIRST + NET_TL_COUNT)

/** First UDP message. */
#define NET_UDP_FIRST  (NET_TL_LAST + 0)

/** Last UDP message. */
#define NET_UDP_LAST  (NET_UDP_FIRST + NET_UDP_COUNT)

/** First TCP message. */
#define NET_TCP_FIRST  (NET_UDP_LAST + 0)

/** Last TCP message. */
#define NET_TCP_LAST  (NET_TCP_FIRST + NET_TCP_COUNT)

/** First socket message. */
#define NET_SOCKET_FIRST  (NET_TCP_LAST + 0)

/** Last socket message. */
#define NET_SOCKET_LAST  (NET_SOCKET_FIRST + NET_SOCKET_COUNT)

/** First packet management system message. */
#define NET_PACKET_FIRST  (NET_SOCKET_LAST + 0)

/** Last packet management system message. */
#define NET_PACKET_LAST  (NET_PACKET_FIRST + NET_PACKET_COUNT)

/** Last networking message. */
#define NET_LAST  NET_PACKET_LAST

/** Number of networking messages. */
#define NET_COUNT  (NET_LAST - NET_FIRST)

/** Check if the IPC call is a generic networking message.
 *
 * @param[in] call IPC call to be checked.
 *
 */
#define IS_NET_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(call), NET_FIRST, NET_LAST)

/** Check if the IPC call is an ARP message.
 *
 * @param[in] call IPC call to be checked.
 *
 */
#define IS_NET_ARP_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(call), NET_ARP_FIRST, NET_ARP_LAST)

/** Check if the IPC call is an Ethernet message.
 *
 * @param[in] call IPC call to be checked.
 *
 */
#define IS_NET_ETH_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(call), NET_ETH_FIRST, NET_ETH_LAST)

/** Check if the IPC call is an ICMP message.
 *
 * @param[in] call IPC call to be checked.
 *
 */
#define IS_NET_ICMP_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(call), NET_ICMP_FIRST, NET_ICMP_LAST)

/** Check if the IPC call is an inter-network layer message.
 *
 * @param[in] call IPC call to be checked.
 *
 */
#define IS_NET_IL_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(call), NET_IL_FIRST, NET_IL_LAST)

/** Check if the IPC call is an IP message.
 *
 * @param[in] call IPC call to be checked.
 *
 */
#define IS_NET_IP_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(call), NET_IP_FIRST, NET_IP_LAST)

/** Check if the IPC call is a generic networking message.
 *
 * @param[in] call IPC call to be checked.
 *
 */
#define IS_NET_NET_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(call), NET_NET_FIRST, NET_NET_LAST)

/** Check if the IPC call is a network interface layer message.
 *
 * @param[in] call IPC call to be checked.
 *
 */
#define IS_NET_NIL_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(call), NET_NIL_FIRST, NET_NIL_LAST)

/** Check if the IPC call is a packet manaagement system message.
 *
 * @param[in] call IPC call to be checked.
 *
 */
#define IS_NET_PACKET_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(call), NET_PACKET_FIRST, NET_PACKET_LAST)

/** Check if the IPC call is a socket message.
 *
 * @param[in] call IPC call to be checked.
 *
 */
#define IS_NET_SOCKET_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(call), NET_SOCKET_FIRST, NET_SOCKET_LAST)

/** Check if the IPC call is a TCP message.
 *
 * @param[in] call IPC call to be checked.
 *
 */
#define IS_NET_TCP_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(call), NET_TCP_FIRST, NET_TCP_LAST)

/** Check if the IPC call is a transport layer message.
 *
 * @param[in] call IPC call to be checked.
 *
 */
#define IS_NET_TL_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(call), NET_TL_FIRST, NET_TL_LAST)

/** Check if the IPC call is a UDP message.
 *
 * @param[in] call IPC call to be checked.
 *
 */
#define IS_NET_UDP_MESSAGE(call) \
	IS_IN_INTERVAL(IPC_GET_IMETHOD(call), NET_UDP_FIRST, NET_UDP_LAST)

/*@}*/

/** @name Networking specific message arguments definitions */
/*@{*/

/** Return the device identifier message argument.
 *
 * @param[in] call Message call structure.
 *
 */
#define IPC_GET_DEVICE(call)  ((nic_device_id_t) IPC_GET_ARG1(call))

/** Return the packet identifier message argument.
 *
 * @param[in] call Message call structure.
 *
 */
#define IPC_GET_PACKET(call)  ((packet_id_t) IPC_GET_ARG2(call))

/** Return the count message argument.
 *
 * @param[in] call Message call structure.
 *
 */
#define IPC_GET_COUNT(call)  ((size_t) IPC_GET_ARG2(call))

/** Return the device state message argument.
 *
 * @param[in] call Message call structure.
 *
 */
#define IPC_GET_STATE(call)  ((nic_device_state_t) IPC_GET_ARG2(call))

/** Return the device handle argument
 *
 * @param[in] call Message call structure
 *
 */
#define IPC_GET_DEVICE_HANDLE(call) ((devman_handle_t) IPC_GET_ARG2(call))

/** Return the device driver service message argument.
 *
 * @param[in] call Message call structure.
 *
 */
#define IPC_GET_SERVICE(call)  ((services_t) IPC_GET_ARG3(call))

/** Return the target service message argument.
 *
 * @param[in] call Message call structure.
 *
 */
#define IPC_GET_TARGET(call)  ((services_t) IPC_GET_ARG3(call))

/** Return the sender service message argument.
 *
 * @param[in] call Message call structure.
 *
 */
#define IPC_GET_SENDER(call)  ((services_t) IPC_GET_ARG3(call))

/** Return the maximum transmission unit message argument.
 *
 * @param[in] call Message call structure.
 *
 */
#define IPC_GET_MTU(call)  ((size_t) IPC_GET_ARG3(call))

/** Return the error service message argument.
 &
 * @param[in] call Message call structure.
 *
 */
#define IPC_GET_ERROR(call)  ((services_t) IPC_GET_ARG4(call))

/** Set the device identifier in the message answer.
 *
 * @param[out] answer Message answer structure.
 * @param[in]  value  Value to set.
 *
 */
#define IPC_SET_DEVICE(answer, value)  IPC_SET_ARG1(answer, (sysarg_t) (value))

/** Set the minimum address length in the message answer.
 *
 * @param[out] answer Message answer structure.
 * @param[in]  value  Value to set.
 *
 */
#define IPC_SET_ADDR(answer, value)  IPC_SET_ARG1(answer, (sysarg_t) (value))

/** Set the minimum prefix size in the message answer.
 *
 * @param[out] answer Message answer structure.
 * @param[in]  value  Value to set.
 *
 */
#define IPC_SET_PREFIX(answer, value)  IPC_SET_ARG2(answer, (sysarg_t) (value))

/** Set the maximum content size in the message answer.
 *
 * @param[out] answer Message answer structure.
 * @param[in]  value  Value to set.
 *
 */
#define IPC_SET_CONTENT(answer, value)  IPC_SET_ARG3(answer, (sysarg_t) (value))

/** Set the minimum suffix size in the message answer.
 *
 * @param[out] answer Message answer structure.
 * @param[in]  value  Value to set.
 *
 */
#define IPC_SET_SUFFIX(answer, value)  IPC_SET_ARG4(answer, (sysarg_t) (value))

/*@}*/

#endif

/** @}
 */
