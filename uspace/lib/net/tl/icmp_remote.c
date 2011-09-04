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
 *  @{
 */

/** @file
 * ICMP interface implementation for remote modules.
 * @see icmp_remote.h
 */

#include <icmp_remote.h>
#include <net/modules.h>
#include <packet_client.h>
#include <ipc/services.h>
#include <ipc/icmp.h>
#include <sys/types.h>
#include <async.h>
#include <errno.h>

/** Send the Destination Unreachable error notification packet.
 *
 * Beginning of the packet is sent as the notification packet data.
 * The source and the destination addresses should be set in the original
 * packet.
 *
 * @param[in] sess   ICMP module session.
 * @param[in] code   Error specific code.
 * @param[in] mtu    Error MTU value.
 * @param[in] packet Original packet.
 *
 * @return EOK on success.
 * @return EPERM if the ICMP error notifications are disabled.
 * @return ENOMEM if there is not enough memory left.
 *
 */
int icmp_destination_unreachable_msg(async_sess_t *sess, icmp_code_t code,
    icmp_param_t mtu, packet_t *packet)
{
	async_exch_t *exch = async_exchange_begin(sess);
	async_msg_3(exch, NET_ICMP_DEST_UNREACH, (sysarg_t) code,
	    (sysarg_t) packet_get_id(packet), (sysarg_t) mtu);
	async_exchange_end(exch);
	
	return EOK;
}

/** Send the Source Quench error notification packet.
 *
 * Beginning of the packet is sent as the notification packet data.
 * The source and the destination addresses should be set in the original
 * packet.
 *
 * @param[in] sess   ICMP module session.
 * @param[in] packet Original packet.
 *
 * @return EOK on success.
 * @return EPERM if the ICMP error notifications are disabled.
 * @return ENOMEM if there is not enough memory left.
 *
 */
int icmp_source_quench_msg(async_sess_t *sess, packet_t *packet)
{
	async_exch_t *exch = async_exchange_begin(sess);
	async_msg_2(exch, NET_ICMP_SOURCE_QUENCH, 0,
	    (sysarg_t) packet_get_id(packet));
	async_exchange_end(exch);
	
	return EOK;
}

/** Send the Time Exceeded error notification packet.
 *
 * Beginning of the packet is sent as the notification packet data.
 * The source and the destination addresses should be set in the original
 * packet.
 *
 * @param[in] sess   ICMP module session.
 * @param[in] code   Error specific code.
 * @param[in] packet Original packet.
 *
 * @return EOK on success.
 * @return EPERM if the ICMP error notifications are disabled.
 * @return ENOMEM if there is not enough memory left.
 *
 */
int icmp_time_exceeded_msg(async_sess_t *sess, icmp_code_t code,
    packet_t *packet)
{
	async_exch_t *exch = async_exchange_begin(sess);
	async_msg_2(exch, NET_ICMP_TIME_EXCEEDED, (sysarg_t) code,
	    (sysarg_t) packet_get_id(packet));
	async_exchange_end(exch);
	
	return EOK;
}

/** Send the Parameter Problem error notification packet.
 *
 * Beginning of the packet is sent as the notification packet data.
 * The source and the destination addresses should be set in the original
 * packet.
 *
 * @param[in] sess    ICMP module session.
 * @param[in] code    Error specific code.
 * @param[in] pointer Problematic parameter offset.
 * @param[in] packet  Original packet.
 *
 * @return EOK on success.
 * @return EPERM if the ICMP error notifications are disabled.
 * @return ENOMEM if there is not enough memory left.
 *
 */
int icmp_parameter_problem_msg(async_sess_t *sess, icmp_code_t code,
    icmp_param_t pointer, packet_t *packet)
{
	async_exch_t *exch = async_exchange_begin(sess);
	async_msg_3(exch, NET_ICMP_PARAMETERPROB, (sysarg_t) code,
	    (sysarg_t) packet_get_id(packet), (sysarg_t) pointer);
	async_exchange_end(exch);
	
	return EOK;
}

/** @}
 */
