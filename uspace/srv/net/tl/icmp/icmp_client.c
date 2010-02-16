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

/** @addtogroup icmp
 *  @{
 */

/** @file
 *  ICMP client interface implementation.
 *  @see icmp_client.h
 */

#include <errno.h>
//#include <stdio.h>

#include <sys/types.h>

#include "../../include/icmp_codes.h"
#include "../../include/icmp_client.h"

#include "../../structures/packet/packet.h"
#include "../../structures/packet/packet_client.h"

#include "icmp_header.h"

int	icmp_client_process_packet( packet_t packet, icmp_type_t * type, icmp_code_t * code, icmp_param_t * pointer, icmp_param_t * mtu ){
	icmp_header_ref	header;

	header = ( icmp_header_ref ) packet_get_data( packet );
	if(( ! header )
	|| ( packet_get_data_length( packet ) < sizeof( icmp_header_t ))){
		return 0;
	}
	if( type ) * type = header->type;
	if( code ) * code = header->code;
	if( pointer ) * pointer = header->un.param.pointer;
	if( mtu ) * mtu = header->un.frag.mtu;
	// remove debug dump
//	printf( "ICMP error %d (%d) in packet %d\n", header->type, header->code, packet_get_id( packet ));
	return sizeof( icmp_header_t );
}

size_t icmp_client_header_length( packet_t packet ){
	if( packet_get_data_length( packet ) < sizeof( icmp_header_t )){
		return 0;
	}
	return sizeof( icmp_header_t );
}

/** @}
 */
