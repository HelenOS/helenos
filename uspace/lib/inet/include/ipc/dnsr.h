/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libinet
 * @{
 */

#ifndef LIBINET_IPC_DNSR_H
#define LIBINET_IPC_DNSR_H

#include <ipc/common.h>

typedef enum {
	DNSR_NAME2HOST = IPC_FIRST_USER_METHOD,
	DNSR_GET_SRVADDR,
	DNSR_SET_SRVADDR
} dnsr_request_t;

#endif

/** @}
 */
