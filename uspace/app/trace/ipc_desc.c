/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
