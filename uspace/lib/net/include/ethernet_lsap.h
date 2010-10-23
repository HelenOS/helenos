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
 * Link service access point identifiers.
 */

#ifndef LIBNET_ETHERNET_LSAP_H_
#define LIBNET_ETHERNET_LSAP_H_

#include <sys/types.h>

/** Ethernet LSAP type definition. */
typedef uint8_t eth_lsap_t;

/** @name Ethernet LSAP values definitions */
/*@{*/

/** Null LSAP LSAP identifier. */
#define ETH_LSAP_NULL	0x00
/** ARPANET Internet Protocol (IP) LSAP identifier. */
#define ETH_LSAP_IP	0x06
/** ARPANET Address Resolution Protocol (ARP) LSAP identifier. */
#define ETH_LSAP_ARP	0x98
/** SubNetwork Access Protocol (SNAP) LSAP identifier. */
#define ETH_LSAP_SNAP	0xAA
/** Global LSAP LSAP identifier. */
#define ETH_LSAP_GLSAP	0xFF

/*@}*/

#endif

/** @}
 */
