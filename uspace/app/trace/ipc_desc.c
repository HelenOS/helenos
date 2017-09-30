/*
 * Copyright (c) 2008 Jiri Svoboda
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

/** @addtogroup trace
 * @{
 */
/** @file
 */

#include <abi/ipc/methods.h>
#include <stdlib.h>
#include "ipc_desc.h"

ipc_m_desc_t ipc_methods[] = {
	/* System methods */
	{ IPC_M_PHONE_HUNGUP,     "PHONE_HUNGUP" },
	{ IPC_M_CONNECT_ME_TO,    "CONNECT_ME_TO" },
	{ IPC_M_CONNECT_TO_ME,    "CONNECT_TO_ME" },
	{ IPC_M_SHARE_OUT,        "SHARE_OUT" },
	{ IPC_M_SHARE_IN,         "SHARE_IN" },
	{ IPC_M_DATA_WRITE,       "DATA_WRITE" },
	{ IPC_M_DATA_READ,        "DATA_READ" },
	{ IPC_M_DEBUG,            "DEBUG" },
};

size_t ipc_methods_len = sizeof(ipc_methods) / sizeof(ipc_m_desc_t);

/** @}
 */
