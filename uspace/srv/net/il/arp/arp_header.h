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

/** @addtogroup arp
 *  @{
 */

/** @file
 *  ARP protocol header.
 *  Based on the RFC~826.
 */

#ifndef __NET_ARP_HEADER_H__
#define __NET_ARP_HEADER_H__

#include <sys/types.h>

/** Type definition of an ARP protocol header.
 *  @see arp_header
 */
typedef struct arp_header	arp_header_t;

/** Type definition of an ARP protocol header pointer.
 *  @see arp_header
 */
typedef arp_header_t *		arp_header_ref;

/** ARP protocol header.
 */
struct arp_header{
	/** Hardware type identifier.
	 *  @see hardware.h
	 */
	uint16_t hardware;
	/** Protocol identifier.
	 */
	uint16_t protocol;
	/** Hardware address length in bytes.
	 */
	uint8_t hardware_length;
	/** Protocol address length in bytes.
	 */
	uint8_t protocol_length;
	/** ARP packet type.
	 *  @see arp_oc.h
	 */
	uint16_t operation;
} __attribute__ ((packed));

#endif

/** @}
 */
