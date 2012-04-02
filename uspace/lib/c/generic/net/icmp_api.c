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
 * ICMP application interface implementation.
 * @see icmp_api.h
 */

#include <net/icmp_api.h>
#include <net/socket_codes.h>
#include <net/inet.h>
#include <net/modules.h>
#include <net/ip_codes.h>
#include <async.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <ipc/services.h>
#include <ipc/icmp.h>

/** Requests an echo message.
 *
 * Sends a packet with specified parameters to the target host and waits for the
 * reply upto the given timeout. Blocks the caller until the reply or the
 * timeout occurs.
 *
 * @param[in] sess The ICMP session.
 * @param[in] size	The message data length in bytes.
 * @param[in] timeout	The timeout in milliseconds.
 * @param[in] ttl	The time to live.
 * @param[in] tos	The type of service.
 * @param[in] dont_fragment The value indicating whether the datagram must not
 *			be fragmented. Is used as a MTU discovery.
 * @param[in] addr	The target host address.
 * @param[in] addrlen	The torget host address length.
 * @return		ICMP_ECHO on success.
 * @return		ETIMEOUT if the reply has not arrived before the
 *			timeout.
 * @return		ICMP type of the received error notification.
 * @return		EINVAL if the addrlen parameter is less or equal to
 *			zero.
 * @return		ENOMEM if there is not enough memory left.
 * @return		EPARTY if there was an internal error.
 */
int
icmp_echo_msg(async_sess_t *sess, size_t size, mseconds_t timeout, ip_ttl_t ttl,
    ip_tos_t tos, int dont_fragment, const struct sockaddr *addr,
    socklen_t addrlen)
{
	aid_t message_id;
	sysarg_t result;

	if (addrlen <= 0)
		return EINVAL;
	
	async_exch_t *exch = async_exchange_begin(sess);
	
	message_id = async_send_5(exch, NET_ICMP_ECHO, size, timeout, ttl,
	    tos, (sysarg_t) dont_fragment, NULL);
	
	/* Send the address */
	async_data_write_start(exch, addr, (size_t) addrlen);
	
	async_exchange_end(exch);

	async_wait_for(message_id, &result);
	return (int) result;
}

/** @}
 */
