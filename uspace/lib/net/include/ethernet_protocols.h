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
 * Ethernet protocol numbers according to the on-line IANA - Ethernet numbers
 * http://www.iana.org/assignments/ethernet-numbers
 * cited January 17 2009.
 */

#ifndef LIBNET_ETHERNET_PROTOCOLS_H_
#define LIBNET_ETHERNET_PROTOCOLS_H_

#include <sys/types.h>

/** Ethernet protocol type definition. */
typedef uint16_t eth_type_t;

/** @name Ethernet protocols definitions */
/*@{*/

/** Ethernet minimal protocol number.
 * According to the IEEE 802.3 specification.
 */
#define ETH_MIN_PROTO		0x0600 /* 1536 */

/** Internet IP (IPv4) ethernet protocol type. */
#define ETH_P_IP		0x0800

/** ARP ethernet protocol type. */
#define ETH_P_ARP		0x0806

/*@}*/

#endif

/** @}
 */
