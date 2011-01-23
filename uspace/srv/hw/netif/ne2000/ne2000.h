/*
 * Copyright (c) 1987,1997, 2006, Vrije Universiteit, Amsterdam, The Netherlands All rights reserved. Redistribution and use of the MINIX 3 operating system in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * * Neither the name of the Vrije Universiteit nor the names of the software authors or contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 * * Any deviations from these conditions require written permission from the copyright holder in advance
 *
 *
 * Disclaimer
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS, AUTHORS, AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR ANY AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Changes:
 *  2009 ported to HelenOS, Lukas Mejdrech
 */

/*
ne2000.h

Created:	March 15, 1994 by Philip Homburg <philip@f-mnx.phicoh.com>
*/

/** @addtogroup ne2k
 *  @{
 */

/** @file
 *  NE1000 and NE2000 network interface definitions.
 */

#ifndef __NET_NETIF_NE2000_H__
#define __NET_NETIF_NE2000_H__

#include <libarch/ddi.h>

#include "dp8390_port.h"

/** DP8390 register offset.
 */
#define NE_DP8390	0x00

/** Data register.
 */
#define NE_DATA		0x10

/** Reset register.
 */
#define NE_RESET	0x1F

/** NE1000 data start.
 */
#define NE1000_START	0x2000

/** NE1000 data size.
 */
#define NE1000_SIZE	0x2000

/** NE2000 data start.
 */
#define NE2000_START	0x4000

/** NE2000 data size.
 */
#define NE2000_SIZE	0x4000

/** Reads 1 byte register.
 *  @param[in] dep The network interface structure.
 *  @param[in] reg The register offset.
 *  @returns The read value.
 */
#define inb_ne(dep, reg)	(inb(dep->de_base_port+reg))

/** Writes 1 byte register.
 *  @param[in] dep The network interface structure.
 *  @param[in] reg The register offset.
 *  @param[in] data The value to be written.
 */
#define outb_ne(dep, reg, data)	(outb(dep->de_base_port+reg, data))

/** Reads 1 word (2 bytes) register.
 *  @param[in] dep The network interface structure.
 *  @param[in] reg The register offset.
 *  @returns The read value.
 */
#define inw_ne(dep, reg)	(inw(dep->de_base_port+reg))

/** Writes 1 word (2 bytes) register.
 *  @param[in] dep The network interface structure.
 *  @param[in] reg The register offset.
 *  @param[in] data The value to be written.
 */
#define outw_ne(dep, reg, data)	(outw(dep->de_base_port+reg, data))

#endif /* __NET_NETIF_NE2000_H__ */

/*
 * $PchId: ne2000.h,v 1.4 2004/08/03 12:03:20 philip Exp $
 */

/** @}
 */
