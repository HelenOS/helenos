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

/** @addtogroup net
 *  @{
 */

/** @file
 *  Generic module functions.
 */

#ifndef __NET_MODULES_H__
#define __NET_MODULES_H__
 
#include <async.h>

#include <ipc/ipc.h>
#include <ipc/services.h>

#include <sys/time.h>

/** Converts the data length between different types.
 *	@param[in] type_from The source type.
 *  @param[in] type_to The destination type.
 *  @param[in] count The number units of the source type size.
 */
#define CONVERT_SIZE(type_from, type_to, count)	((sizeof(type_from) / sizeof(type_to)) * (count))

/** Registers the module service at the name server.
 *  @param[in] me The module service.
 *  @param[out] phonehash The created phone hash.
 */
#define REGISTER_ME(me, phonehash)	ipc_connect_to_me(PHONE_NS, (me), 0, 0, (phonehash))

/** Connect to the needed module function type definition.
 *  @param[in] need The needed module service.
 *  @returns The phone of the needed service.
 */
typedef int connect_module_t(services_t need);

/** Answers the call.
 *  @param[in] callid The call identifier.
 *  @param[in] result The message processing result.
 *  @param[in] answer The message processing answer.
 *  @param[in] answer_count The number of answer parameters.
 */
extern void answer_call(ipc_callid_t callid, int result, ipc_call_t * answer, int answer_count);

extern int bind_service(services_t, ipcarg_t, ipcarg_t, ipcarg_t,
    async_client_conn_t);
extern int bind_service_timeout(services_t, ipcarg_t, ipcarg_t, ipcarg_t,
    async_client_conn_t, suseconds_t);

/** Connects to the needed module.
 *  @param[in] need The needed module service.
 *  @returns The phone of the needed service.
 */
extern int connect_to_service(services_t need);

/** Connects to the needed module.
 *  @param[in] need The needed module service.
 *  @param[in] timeout The connection timeout in microseconds. No timeout if set to zero (0).
 *  @returns The phone of the needed service.
 *  @returns ETIMEOUT if the connection timeouted.
 */
extern int connect_to_service_timeout(services_t need, suseconds_t timeout);

/** Receives data from the other party.
 *  The received data buffer is allocated and returned.
 *  @param[out] data The data buffer to be filled.
 *  @param[out] length The buffer length.
 *  @returns EOK on success.
 *  @returns EBADMEM if the data or the length parameter is NULL.
 *  @returns EINVAL if the client does not send data.
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns Other error codes as defined for the async_data_write_finalize() function.
 */
extern int data_receive(void ** data, size_t * length);

/** Replies the data to the other party.
 *  @param[in] data The data buffer to be sent.
 *  @param[in] data_length The buffer length.
 *  @returns EOK on success.
 *  @returns EINVAL if the client does not expect the data.
 *  @returns EOVERFLOW if the client does not expect all the data. Only partial data are transfered.
 *  @returns Other error codes as defined for the async_data_read_finalize() function.
 */
extern int data_reply(void * data, size_t data_length);

/** Refreshes answer structure and parameters count.
 *  Erases all attributes.
 *  @param[in,out] answer The message processing answer structure.
 *  @param[in,out] answer_count The number of answer parameters.
 */
extern void refresh_answer(ipc_call_t * answer, int * answer_count);

#endif

/** @}
 */
