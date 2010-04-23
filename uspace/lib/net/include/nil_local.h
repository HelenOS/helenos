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

/** @addtogroup net_nil
 *  @{
 */

/** @file
 * Network interface layer modules common skeleton.
 * All network interface layer modules have to implement this interface.
 */

#ifndef __NET_NIL_LOCAL_H__
#define __NET_NIL_LOCAL_H__

#include <ipc/ipc.h>

/** Module initialization.
 *
 * Is called by the module_start() function.
 *
 * @param[in] net_phone The networking moduel phone.
 *
 * @return EOK on success.
 * @return Other error codes as defined for each specific module initialize function.
 *
 */
extern int nil_initialize(int);

extern int nil_device_state_msg_local(int, device_id_t, int);
extern int nil_received_msg_local(int, device_id_t, packet_t, services_t);

/** Message processing function.
 *
 * @param[in]  name         Module name.
 * @param[in]  callid       The message identifier.
 * @param[in]  call         The message parameters.
 * @param[out] answer       The message answer parameters.
 * @param[out] answer_count The last parameter for the actual answer
 *                          in the answer parameter.
 *
 * @return EOK on success.
 * @return ENOTSUP if the message is not known.
 * @return Other error codes as defined for each specific
 *         module message function.
 *
 * @see nil_interface.h
 * @see IS_NET_NIL_MESSAGE()
 *
 */
extern int nil_message_standalone(const char *, ipc_callid_t, ipc_call_t *, ipc_call_t *,
    int *);

extern int nil_module_message_standalone(const char *, ipc_callid_t,
    ipc_call_t *, ipc_call_t *, int *);
extern int nil_module_start_standalone(async_client_conn_t);

#endif

/** @}
 */
