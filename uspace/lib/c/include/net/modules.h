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
 * Generic module functions.
 *
 * @todo MAKE IT POSSIBLE TO REMOVE THIS FILE VIA EITHER REPLACING PART OF ITS
 * FUNCTIONALITY OR VIA INTEGRATING ITS FUNCTIONALITY MORE TIGHTLY WITH THE REST
 * OF THE SYSTEM.
 */

#ifndef LIBC_MODULES_H_
#define LIBC_MODULES_H_

#include <async.h>

#include <ipc/ipc.h>
#include <ipc/services.h>

#include <sys/time.h>

/** Converts the data length between different types.
 *
 * @param[in] type_from	The source type.
 * @param[in] type_to	The destination type.
 * @param[in] count	The number units of the source type size.
 */
#define CONVERT_SIZE(type_from, type_to, count) \
	((sizeof(type_from) / sizeof(type_to)) * (count))

/** Registers the module service at the name server.
 *
 * @param[in] me	The module service.
 * @param[out] phonehash The created phone hash.
 */
#define REGISTER_ME(me, phonehash) \
	ipc_connect_to_me(PHONE_NS, (me), 0, 0, (phonehash))

/** Connect to the needed module function type definition.
 *
 * @param[in] need	The needed module service.
 * @return		The phone of the needed service.
 */
typedef int connect_module_t(services_t need);

extern void answer_call(ipc_callid_t, int, ipc_call_t *, int);
extern int bind_service(services_t, sysarg_t, sysarg_t, sysarg_t,
    async_client_conn_t);
extern int bind_service_timeout(services_t, sysarg_t, sysarg_t, sysarg_t,
    async_client_conn_t, suseconds_t);
extern int connect_to_service(services_t);
extern int connect_to_service_timeout(services_t, suseconds_t);
extern int data_receive(void **, size_t *);
extern int data_reply(void *, size_t);
extern void refresh_answer(ipc_call_t *, int *);

#endif

/** @}
 */
