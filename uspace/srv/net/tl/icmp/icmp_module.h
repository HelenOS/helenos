/*
 * Copyright (c) 2008 Lukas Mejdrech
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
 *  ICMP module functions.
 *  The functions are used as ICMP module entry points.
 */

#ifndef __NET_ICMP_MODULE_H__
#define __NET_ICMP_MODULE_H__

#include <async.h>
#include <ipc/ipc.h>

/** Initializes the ICMP module.
 *  @param[in] client_connection The client connection processing function. The module skeleton propagates its own one.
 *  @returns EOK on success.
 *  @returns ENOMEM if there is not enough memory left.
 */
int	icmp_initialize( async_client_conn_t client_connection );

/** Processes the ICMP message.
 *  @param[in] callid The message identifier.
 *  @param[in] call The message parameters.
 *  @param[out] answer The message answer parameters.
 *  @param[out] answer_count The last parameter for the actual answer in the answer parameter.
 *  @returns EOK on success.
 *  @returns ENOTSUP if the message is not known.
 *  @see icmp_interface.h
 *  @see IS_NET_ICMP_MESSAGE()
 */
int	icmp_message( ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count );

#endif

/** @}
 */
