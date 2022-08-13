/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libinet
 * @{
 */
/** @file
 */

#ifndef LIBINET_IPC_UDP_H
#define LIBINET_IPC_UDP_H

#include <ipc/common.h>

typedef enum {
	UDP_CALLBACK_CREATE = IPC_FIRST_USER_METHOD,
	UDP_ASSOC_CREATE,
	UDP_ASSOC_DESTROY,
	UDP_ASSOC_SET_NOLOCAL,
	UDP_ASSOC_SEND_MSG,
	UDP_RMSG_INFO,
	UDP_RMSG_READ,
	UDP_RMSG_DISCARD
} udp_request_t;

typedef enum {
	UDP_EV_DATA = IPC_FIRST_USER_METHOD
} udp_event_t;

#endif

/** @}
 */
