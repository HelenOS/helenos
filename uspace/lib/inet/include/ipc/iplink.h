/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libinet
 * @{
 */
/** @file
 */

#ifndef LIBINET_IPC_IPLINK_H
#define LIBINET_IPC_IPLINK_H

#include <ipc/common.h>

typedef enum {
	IPLINK_GET_MTU = IPC_FIRST_USER_METHOD,
	IPLINK_GET_MAC48,
	IPLINK_SET_MAC48,
	IPLINK_SEND,
	IPLINK_SEND6,
	IPLINK_ADDR_ADD,
	IPLINK_ADDR_REMOVE
} iplink_request_t;

typedef enum {
	IPLINK_EV_RECV = IPC_FIRST_USER_METHOD,
	IPLINK_EV_CHANGE_ADDR,
} iplink_event_t;

#endif

/**
 * @}
 */
