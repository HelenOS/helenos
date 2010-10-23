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

/** @addtogroup libc
 *  @{
 */

/** @file
 * ICMP types and codes according to the on-line IANA - ICMP Type Numbers
 *
 * http://www.iana.org/assignments/icmp-parameters>
 *
 * cited September 14 2009.
 */

#ifndef LIBC_ICMP_CODES_H_
#define LIBC_ICMP_CODES_H_

#include <sys/types.h>

/** ICMP type type definition. */
typedef uint8_t icmp_type_t;

/** ICMP code type definition. */
typedef uint8_t icmp_code_t;

/** ICMP parameter type definition. */
typedef uint16_t icmp_param_t;

/** @name ICMP types definitions */
/*@{*/

/** Echo Reply. */
#define ICMP_ECHOREPLY		0

/** Destination Unreachable. */
#define ICMP_DEST_UNREACH	3

/** Source Quench. */
#define ICMP_SOURCE_QUENCH	4

/** Redirect. */
#define ICMP_REDIRECT		5

/** Alternate Host Address. */
#define ICMP_ALTERNATE_ADDR	6

/** Echo Request. */
#define ICMP_ECHO		8

/** Router Advertisement. */
#define ICMP_ROUTER_ADV		9

/** Router solicitation. */
#define ICMP_ROUTER_SOL		10

/** Time Exceeded. */
#define ICMP_TIME_EXCEEDED	11

/** Parameter Problem. */
#define ICMP_PARAMETERPROB	12

/** Timestamp Request. */
#define ICMP_TIMESTAMP		13

/** Timestamp Reply. */
#define ICMP_TIMESTAMPREPLY	14

/** Information Request. */
#define ICMP_INFO_REQUEST	15

/** Information Reply. */
#define ICMP_INFO_REPLY		16

/** Address Mask Request. */
#define ICMP_ADDRESS		17

/** Address Mask Reply. */
#define ICMP_ADDRESSREPLY	18

/** Traceroute. */
#define ICMP_TRACEROUTE		30

/** Datagram Conversion Error. */
#define ICMP_CONVERSION_ERROR	31

/** Mobile Host Redirect. */
#define ICMP_REDIRECT_MOBILE	32

/** IPv6 Where-Are-You. */
#define ICMP_IPV6_WHERE_ARE_YOU	33

/** IPv6 I-Am-Here. */
#define ICMP_IPV6_I_AM_HERE	34

/** Mobile Registration Request. */
#define ICMP_MOBILE_REQUEST	35

/** Mobile Registration Reply. */
#define ICMP_MOBILE_REPLY	36

/** Domain name request. */
#define ICMP_DN_REQUEST		37

/** Domain name reply. */
#define ICMP_DN_REPLY		38

/** SKIP. */
#define ICMP_SKIP		39

/** Photuris. */
#define ICMP_PHOTURIS		40

/*@}*/

/** @name ICMP_DEST_UNREACH codes definitions
 */
/*@{*/

/** Network Unreachable. */
#define ICMP_NET_UNREACH	0

/** Host Unreachable. */
#define ICMP_HOST_UNREACH	1

/** Protocol Unreachable. */
#define ICMP_PROT_UNREACH	2

/** Port Unreachable. */
#define ICMP_PORT_UNREACH	3

/** Fragmentation needed but the Do Not Fragment bit was set. */
#define ICMP_FRAG_NEEDED	4

/** Source Route failed. */
#define ICMP_SR_FAILED		5

/** Destination network unknown. */
#define ICMP_NET_UNKNOWN	6

/** Destination host unknown. */
#define ICMP_HOST_UNKNOWN	7

/** Source host isolated (obsolete). */
#define ICMP_HOST_ISOLATED	8

/** Destination network administratively prohibited. */
#define ICMP_NET_ANO		9

/** Destination host administratively prohibited. */
#define ICMP_HOST_ANO		10

/** Network unreachable for this type of service. */
#define ICMP_NET_UNR_TOS	11

/** Host unreachable for this type of service. */
#define ICMP_HOST_UNR_TOS	12

/** Communication administratively prohibited by filtering. */
#define ICMP_PKT_FILTERED	13

/** Host precedence violation. */
#define ICMP_PREC_VIOLATION	14

/** Precedence cutoff in effect. */
#define ICMP_PREC_CUTOFF	15

/*@}*/

/** @name ICMP_REDIRECT codes definitions */
/*@{*/

/** Network redirect (or subnet). */
#define ICMP_REDIR_NET		0

/** Host redirect. */
#define ICMP_REDIR_HOST		1

/** Network redirect for this type of service. */
#define ICMP_REDIR_NETTOS	2

/** Host redirect for this type of service. */
#define ICMP_REDIR_HOSTTOS	3

/*@}*/

/** @name ICMP_ALTERNATE_ADDRESS codes definitions */
/*@{*/

/** Alternate address for host. */
#define ICMP_ALTERNATE_HOST	0

/*@}*/

/** @name ICMP_ROUTER_ADV codes definitions */
/*@{*/

/** Normal router advertisement. */
#define ICMP_ROUTER_NORMAL	0

/** Does not route common traffic. */
#define ICMP_ROUTER_NO_NORMAL_TRAFFIC	16

/*@}*/

/** @name ICMP_TIME_EXCEEDED codes definitions */
/*@{*/

/** Transit TTL exceeded. */
#define ICMP_EXC_TTL		0

/** Reassembly TTL exceeded. */
#define ICMP_EXC_FRAGTIME	1

/*@}*/

/** @name ICMP_PARAMETERPROB codes definitions */
/*@{*/

/** Pointer indicates the error. */
#define ICMP_PARAM_POINTER	0

/** Missing required option. */
#define ICMP_PARAM_MISSING	1

/** Bad length. */
#define ICMP_PARAM_LENGTH	2

/*@}*/

/** @name ICMP_PHOTURIS codes definitions */
/*@{*/

/** Bad SPI. */
#define ICMP_PHOTURIS_BAD_SPI			0

/** Authentication failed. */
#define ICMP_PHOTURIS_AUTHENTICATION		1

/** Decompression failed. */
#define ICMP_PHOTURIS_DECOMPRESSION		2

/** Decryption failed. */
#define ICMP_PHOTURIS_DECRYPTION		3

/** Need authentication. */
#define ICMP_PHOTURIS_NEED_AUTHENTICATION	4

/** Need authorization. */
#define ICMP_PHOTURIS_NEED_AUTHORIZATION	5

/*@}*/

#endif

/** @}
 */
