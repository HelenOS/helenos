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
 * @{
 */

/** @file
 * Network interface common module messages.
 */

#ifndef LIBC_NETIF_MESSAGES_H_
#define LIBC_NETIF_MESSAGES_H_

#include <ipc/net.h>

/** Network interface common module messages. */
typedef enum {
	/** Probe device message.
	 * @see netif_probe_req()
	 */
	NET_NETIF_PROBE = NET_NETIF_FIRST,
	
	/** Send packet message.
	 * @see netif_send_msg()
	 */
	NET_NETIF_SEND,
	
	/** Start device message.
	 * @see netif_start_req()
	 */
	NET_NETIF_START,
	
	/** Get device usage statistics message.
	 * @see netif_stats_req()
	 */
	NET_NETIF_STATS,
	
	/** Stop device message.
	 * @see netif_stop_req()
	 */
	NET_NETIF_STOP,
	
	/** Get device address message.
	 * @see netif_get_addr_req()
	 */
	NET_NETIF_GET_ADDR,
} netif_messages;

/** @name Network interface specific message parameters definitions */
/*@{*/

/** Return the interrupt number message parameter.
 *
 * @param[in] call Mmessage call structure.
 *
 */
#define NETIF_GET_IRQ(call) ((int) IPC_GET_ARG2(call))

/** Return the input/output address message parameter.
 *
 * @param[in] call Message call structure.
 *
 */
#define NETIF_GET_IO(call) ((void *) IPC_GET_ARG3(call))

/*@}*/

#endif

/** @}
 */
