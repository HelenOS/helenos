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

/** @addtogroup eth
 *  @{
 */

/** @file
 *  Link service access point identifiers.
 */

#ifndef __NET_ETHERNET_LSAP_H__
#define __NET_ETHERNET_LSAP_H__

#include <sys/types.h>

/** Ethernet LSAP type definition.
 */
typedef uint8_t	eth_lsap_t;

/** @name Ethernet LSAP values definitions
 */
/*@{*/

/** Null LSAP LSAP identifier.
 */
#define ETH_LSAP_NULL	0x00
/** Individual LLC Sublayer Management Function LSAP identifier.
 */
#define ETH_LSAP_ISLMF	0x02
/** Group LLC Sublayer Management Function LSAP identifier.
 */
#define ETH_LSAP_GSLMI	0x03
/** IBM SNA Path Control (individual) LSAP identifier.
 */
#define ETH_LSAP_ISNA	0x04
/** IBM SNA Path Control (group) LSAP identifier.
 */
#define ETH_LSAP_GSNA	0x05
/** ARPANET Internet Protocol (IP) LSAP identifier.
 */
#define ETH_LSAP_IP	0x06
/** SNA LSAP identifier.
 */
#define ETH_LSAP_SNA	0x08
/** SNA LSAP identifier.
 */
#define ETH_LSAP_SNA2	0x0C
/** PROWAY (IEC955) Network Management &Initialization LSAP identifier.
 */
#define ETH_LSAP_PROWAY_NMI	0x0E
/** Texas Instruments LSAP identifier.
 */
#define ETH_LSAP_TI	0x18
/** IEEE 802.1 Bridge Spanning Tree Protocol LSAP identifier.
 */
#define ETH_LSAP_BRIDGE	0x42
/** EIA RS-511 Manufacturing Message Service LSAP identifier.
 */
#define ETH_LSAP_EIS	0x4E
/** ISO 8208 (X.25 over IEEE 802.2 Type 2 LLC) LSAP identifier.
 */
#define ETH_LSAP_ISO8208	0x7E
/** Xerox Network Systems (XNS) LSAP identifier.
 */
#define ETH_LSAP_XNS	0x80
/** Nestar LSAP identifier.
 */
#define ETH_LSAP_NESTAR	0x86
/** PROWAY (IEC 955) Active Station List Maintenance LSAP identifier.
 */
#define ETH_LSAP_PROWAY_ASLM	0x8E
/** ARPANET Address Resolution Protocol (ARP) LSAP identifier.
 */
#define ETH_LSAP_ARP	0x98
/** Banyan VINES LSAP identifier.
 */
#define ETH_LSAP_VINES	0xBC
/** SubNetwork Access Protocol (SNAP) LSAP identifier.
 */
#define ETH_LSAP_SNAP	0xAA
/** Novell NetWare LSAP identifier.
 */
#define ETH_LSAP_NETWARE	0xE0
/** IBM NetBIOS LSAP identifier.
 */
#define ETH_LSAP_NETBIOS	0xF0
/** IBM LAN Management (individual) LSAP identifier.
 */
#define ETH_LSAP_ILAN	0xF4
/** IBM LAN Management (group) LSAP identifier.
 */
#define ETH_LSAP_GLAN	0xF5
/** IBM Remote Program Load (RPL) LSAP identifier.
 */
#define ETH_LSAP_RPL	0xF8
/** Ungermann-Bass LSAP identifier.
 */
#define ETH_LSAP_UB	0xFA
/** ISO Network Layer Protocol LSAP identifier.
 */
#define ETH_LSAP_ISONLP	0xFE
/** Global LSAP LSAP identifier.
 */
#define ETH_LSAP_GLSAP	0xFF

/*@}*/

#endif

/** @}
 */
