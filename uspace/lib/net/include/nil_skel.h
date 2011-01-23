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
 * @{
 */

/** @file
 * Network interface layer modules common skeleton.
 * All network interface layer modules have to implement this interface.
 */

#ifndef LIBNET_NIL_SKEL_H_
#define LIBNET_NIL_SKEL_H_

#include <async.h>
#include <fibril_synch.h>
#include <ipc/ipc.h>
#include <ipc/services.h>

#include <adt/measured_strings.h>
#include <net/device.h>
#include <net/packet.h>

/** Module initialization.
 *
 * This has to be implemented in user code.
 *
 * @param[in] net_phone Networking module phone.
 *
 * @return EOK on success.
 * @return Other error codes as defined for each specific module
 *         initialize function.
 *
 */
extern int nil_initialize(int net_phone);

/** Notify the network interface layer about the device state change.
 *
 * This has to be implemented in user code.
 *
 * @param[in] nil_phone Network interface layer phone.
 * @param[in] device_id Device identifier.
 * @param[in] state     New device state.
 *
 * @return EOK on success.
 * @return Other error codes as defined for each specific module
 *         device state function.
 *
 */
extern int nil_device_state_msg_local(int, device_id_t, int);

/** Pass the packet queue to the network interface layer.
 *
 * Process and redistribute the received packet queue to the registered
 * upper layers.
 *
 * This has to be implemented in user code.
 *
 * @param[in] nil_phone Network interface layer phone.
 * @param[in] device_id Source device identifier.
 * @param[in] packet    Received packet or the received packet queue.
 * @param[in] target    Target service. Ignored parameter.
 *
 * @return EOK on success.
 * @return Other error codes as defined for each specific module
 *         received function.
 *
 */
extern int nil_received_msg_local(int, device_id_t, packet_t *, services_t);

/** Message processing function.
 *
 * This has to be implemented in user code.
 *
 * @param[in]  name   Module name.
 * @param[in]  callid Message identifier.
 * @param[in]  call   Message parameters.
 * @param[out] answer Message answer parameters.
 * @param[out] count  Message answer arguments.
 *
 * @return EOK on success.
 * @return ENOTSUP if the message is not known.
 * @return Other error codes as defined for each specific module
 *         message function.
 *
 * @see IS_NET_NIL_MESSAGE()
 *
 */
extern int nil_module_message(ipc_callid_t, ipc_call_t *, ipc_call_t *,
    size_t *);

extern int nil_module_start(int);

#endif

/** @}
 */
