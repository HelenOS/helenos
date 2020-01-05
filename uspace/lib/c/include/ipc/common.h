/*
 * Copyright (c) 2011 Martin Decky
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

/** @addtogroup libcipc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IPC_COMMON_H_
#define _LIBC_IPC_COMMON_H_

#include <types/common.h>
#include <abi/ipc/ipc.h>

/* Well known phone descriptors */
static cap_phone_handle_t const PHONE_INITIAL = (cap_phone_handle_t) (CAP_NIL + 1);
#define IPC_FLAG_BLOCKING   0x01

/**
 * IPC_FLAG_AUTOSTART_ is for use in brokers only. In client code use
 * IPC_AUTOSTART that includes implies blocking behavior.
 */
#define IPC_FLAG_AUTOSTART_  0x02

/**
 * Similar to blocking IPC_FLAG_BLOCKING behavior, broker will attempt to
 * start the server.
 */
#define IPC_AUTOSTART (IPC_FLAG_BLOCKING | IPC_FLAG_AUTOSTART_)

typedef ipc_data_t ipc_call_t;

#endif

/** @}
 */
