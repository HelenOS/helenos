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

/** @addtogroup socket
 *  @{
 */

/** @file
 *  Socket codes and definitions.
 *  This is a part of the network application library.
 */

#ifndef __NET_SOCKET_CODES_H__
#define __NET_SOCKET_CODES_H__

#include <sys/types.h>

/** @name Address families definitions
 */
/*@{*/

/** Unspecified address family.
 */
#define AF_UNSPEC	0

/** Unix domain sockets address family.
 */
#define AF_UNIXL	1

/** POSIX name for AF_UNIX address family.
 */
#define AF_LOCAL	1

/** Internet IP Protocol address family.
 */
#define AF_INET		2

/** Amateur Radio AX.25 address family.
 */
#define AF_AX25		3

/** Novell IPX address family.
 */
#define AF_IPX		4

/** AppleTalk DDP address family.
 */
#define AF_APPLETALK	5

/** Amateur Radio NET/ROM address family.
 */
#define AF_NETROM	6

/** Multiprotocol bridge address family.
 */
#define AF_BRIDGE	7

/** ATM PVCs address family.
 */
#define AF_ATMPVC	8

/** Reserved for X.25 project address family.
 */
#define AF_X25		9

/** IP version 6 address family.
 */
#define AF_INET6	10

/** Amateur Radio X.25 PLP address family.
 */
#define AF_ROSE		11

/** Reserved for DECnet project address family.
 */
#define AF_DECnet	12

/** Reserved for 802.2LLC project address family.
 */
#define AF_NETBEUI	13

/** Security callback pseudo AF address family.
 */
#define AF_SECURITY	14

/** PF_KEY key management API address family.
 */
#define AF_KEY		15

/** Alias to emulate 4.4BSD address family.
 */
#define AF_NETLINK	16

/** Packet family address family.
 */
#define AF_PACKET	17

/** Ash address family.
 */
#define AF_ASH		18

/** Acorn Econet address family.
 */
#define AF_ECONET	19

/** ATM SVCs address family.
 */
#define AF_ATMSVC	20

/** Linux SNA Project (nutters!) address family.
 */
#define AF_SNA		22

/** IRDA sockets address family.
 */
#define AF_IRDA		23

/** PPPoX sockets address family.
 */
#define AF_PPPOX	24

/** Wanpipe API Sockets address family.
 */
#define AF_WANPIPE	25

/** Linux LLC address family.
 */
#define AF_LLC		26

/** Controller Area Network address family.
 */
#define AF_CAN		29

/** TIPC sockets address family.
 */
#define AF_TIPC		30

/** Bluetooth sockets address family.
 */
#define AF_BLUETOOTH	31

/** IUCV sockets address family.
 */
#define AF_IUCV		32

/** RxRPC sockets address family.
 */
#define AF_RXRPC	33

/** Maximum address family.
 */
#define AF_MAX		34

/*@}*/

/** @name Protocol families definitions
 *  Same as address families.
 */
/*@{*/

/** Unspecified protocol family.
 */
#define PF_UNSPEC	AF_UNSPEC

/** Unix domain sockets protocol family.
 */
#define PF_UNIXL	AF_UNIXL

/** POSIX name for AF_UNIX protocol family.
 */
#define PF_LOCAL	AF_LOCAL

/** Internet IP Protocol protocol family.
 */
#define PF_INET		AF_INET

/** Amateur Radio AX.25 protocol family.
 */
#define PF_AX25		AF_AX25

/** Novell IPX protocol family.
 */
#define PF_IPX		AF_IPX

/** AppleTalk DDP protocol family.
 */
#define PF_APPLETALK	AF_APPLETALK

/** Amateur Radio NET/ROM protocol family.
 */
#define PF_NETROM	AF_NETROM

/** Multiprotocol bridge protocol family.
 */
#define PF_BRIDGE	AF_BRIDGE

/** ATM PVCs protocol family.
 */
#define PF_ATMPVC	AF_ATMPVC

/** Reserved for X.25 project protocol family.
 */
#define PF_X25		AF_X25

/** IP version 6 protocol family.
 */
#define PF_INET6	AF_INET6

/** Amateur Radio X.25 PLP protocol family.
 */
#define PF_ROSE		AF_ROSE

/** Reserved for DECnet project protocol family.
 */
#define PF_DECnet	AF_DECnet

/** Reserved for 802.2LLC project protocol family.
 */
#define PF_NETBEUI	AF_NETBEUI

/** Security callback pseudo AF protocol family.
 */
#define PF_SECURITY	AF_SECURITY

/** PF_KEY key management API protocol family.
 */
#define PF_KEY		AF_KEY

/** Alias to emulate 4.4BSD protocol family.
 */
#define PF_NETLINK	AF_NETLINK

/** Packet family protocol family.
 */
#define PF_PACKET	AF_PACKET

/** Ash protocol family.
 */
#define PF_ASH		AF_ASH

/** Acorn Econet protocol family.
 */
#define PF_ECONET	AF_ECONET

/** ATM SVCs protocol family.
 */
#define PF_ATMSVC	AF_ATMSVC

/** Linux SNA Project (nutters!) protocol family.
 */
#define PF_SNA		AF_SNA

/** IRDA sockets protocol family.
 */
#define PF_IRDA		AF_IRDA

/** PPPoX sockets protocol family.
 */
#define PF_PPPOX	AF_PPPOX

/** Wanpipe API Sockets protocol family.
 */
#define PF_WANPIPE	AF_WANPIPE

/** Linux LLC protocol family.
 */
#define PF_LLC		AF_LLC

/** Controller Area Network protocol family.
 */
#define PF_CAN		AF_CAN

/** TIPC sockets protocol family.
 */
#define PF_TIPC		AF_TIPC

/** Bluetooth sockets protocol family.
 */
#define PF_BLUETOOTH	AF_BLUETOOTH

/** IUCV sockets protocol family.
 */
#define PF_IUCV		AF_IUCV

/** RxRPC sockets protocol family.
 */
#define PF_RXRPC	AF_RXRPC

/** Maximum protocol family.
 */
#define PF_MAX		AF_MAX

/*@}*/

/** @name Socket option levels definitions
 *  Thanks to BSD these must match IPPROTO_xxx
 */
/*@{*/

/** IP socket option level.
 */
#define SOL_IP		0

/** ICMP socket option level.
 */
#define SOL_ICMP	1

/** TCP socket option level.
 */
#define SOL_TCP		6

/** UDP socket option level.
 */
#define SOL_UDP		17

/** IPV socket option level.
 */
#define SOL_IPV6	41

/** ICMPV socket option level.
 */
#define SOL_ICMPV6	58

/** SCTP socket option level.
 */
#define SOL_SCTP	132

/** UDP-Lite (RFC 3828) socket option level.
 */
#define SOL_UDPLITE	136

/** RAW socket option level.
 */
#define SOL_RAW		255

/** IPX socket option level.
 */
#define SOL_IPX		256

/** AX socket option level.
 */
#define SOL_AX25	257

/** ATALK socket option level.
 */
#define SOL_ATALK	258

/** NETROM socket option level.
 */
#define SOL_NETROM	259

/** ROSE socket option level.
 */
#define SOL_ROSE	260

/** DECNET socket option level.
 */
#define SOL_DECNET	261

/** X25 socket option level.
 */
#define	SOL_X25		262

/** PACKET socket option level.
 */
#define SOL_PACKET	263

/** ATM layer (cell level) socket option level.
 */
#define SOL_ATM		264

/** ATM Adaption Layer (packet level) socket option level.
 */
#define SOL_AAL		265

/** IRDA socket option level.
 */
#define SOL_IRDA	266

/** NETBEUI socket option level.
 */
#define SOL_NETBEUI	267

/** LLC socket option level.
 */
#define SOL_LLC		268

/** DCCP socket option level.
 */
#define SOL_DCCP	269

/** NETLINK socket option level.
 */
#define SOL_NETLINK	270

/** TIPC socket option level.
 */
#define SOL_TIPC	271

/** RXRPC socket option level.
 */
#define SOL_RXRPC	272

/** PPPOL socket option level.
 */
#define SOL_PPPOL2TP	273

/** BLUETOOTH socket option level.
 */
#define SOL_BLUETOOTH	274

/*@}*/

/** Socket types.
 */
typedef enum sock_type{
	/** Stream (connection oriented) socket.
	 */
	SOCK_STREAM	= 1,
	/** Datagram (connectionless oriented) socket.
	 */
	SOCK_DGRAM	= 2,
	/** Raw socket.
	 */
	SOCK_RAW	= 3
} sock_type_t;

/** Type definition of the socket length.
 */
typedef int32_t	socklen_t;

#endif

/** @}
 */
