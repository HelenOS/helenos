/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libinet
 * @{
 */
/** @file
 */

#ifndef LIBINET_IPC_DHCP_H
#define LIBINET_IPC_DHCP_H

#include <ipc/common.h>

/** DHCP service requests */
typedef enum {
	DHCP_LINK_ADD = IPC_FIRST_USER_METHOD,
	DHCP_LINK_REMOVE,
	DHCP_DISCOVER
} dhcp_request_t;

#endif

/**
 * @}
 */
