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
 *  Packet server module messages.
 */

#ifndef LIBC_PACKET_MESSAGES_
#define LIBC_PACKET_MESSAGES_

#include <ipc/net.h>

/** Packet server module messages. */
typedef enum {
	/** Create packet message with specified content length.
	 * @see packet_get_1()
	 */
	NET_PACKET_CREATE_1 = NET_PACKET_FIRST,
	
	/**
	 * Create packet message with specified address length, prefix, content
	 * and suffix.
	 * @see packet_get_4()
	 */
	NET_PACKET_CREATE_4,
	
	/** Get packet message.
	 * @see packet_return() */
	NET_PACKET_GET,
	
	/** Get packet size message.
	 * @see packet_translate()
	 */
	NET_PACKET_GET_SIZE,
	
	/** Release packet message.
	 * @see pq_release()
	 */
	NET_PACKET_RELEASE
} packet_messages;

/** Return the protocol service message parameter. */
#define ARP_GET_PROTO(call)  ((services_t) IPC_GET_ARG2(call))

/** Return the packet identifier message parameter. */
#define IPC_GET_ID(call)  ((packet_id_t) IPC_GET_ARG1(call))

/** Return the maximal content length message parameter. */
#define IPC_GET_CONTENT(call)  ((size_t) IPC_GET_ARG1(call))

/** Return the maximal address length message parameter. */
#define IPC_GET_ADDR_LEN(call)  ((size_t) IPC_GET_ARG2(call))

/** Return the maximal prefix length message parameter. */
#define IPC_GET_PREFIX(call)  ((size_t) IPC_GET_ARG3(call))

/** Return the maximal suffix length message parameter. */
#define IPC_GET_SUFFIX(call)  ((size_t) IPC_GET_ARG4(call))

#endif

/** @}
 */
