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
 *  ICMP application interface implementation.
 *  @see icmp_api.h
 */

#include <async.h>

#include <ipc/ipc.h>
#include <ipc/services.h>

#include <sys/types.h>

#include "../../modules.h"

#include "../../include/icmp_api.h"
#include "../../include/inet.h"
#include "../../include/ip_codes.h"
#include "../../include/socket_codes.h"

#include "icmp_messages.h"

int icmp_echo_msg( int icmp_phone, size_t size, mseconds_t timeout, ip_ttl_t ttl, ip_tos_t tos, int dont_fragment, const struct sockaddr * addr, socklen_t addrlen ){
	aid_t			message_id;
	ipcarg_t		result;

	if( addrlen <= 0 ){
		return EINVAL;
	}
	message_id = async_send_5( icmp_phone, NET_ICMP_ECHO, size, timeout, ttl, tos, ( ipcarg_t ) dont_fragment, NULL );
	// send the address
	async_data_write_start( icmp_phone, addr, ( size_t ) addrlen );
	// timeout version may cause inconsistency - there is also an inner timer
	// return async_wait_timeout( message_id, & result, timeout );
	async_wait_for( message_id, & result );
	return ( int ) result;
}

/** @}
 */
