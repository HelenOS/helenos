/*
 * SPDX-FileCopyrightText: 2009 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 */

#ifndef LIBDEVICE_IPC_IRC_H
#define LIBDEVICE_IPC_IRC_H

#include <ipc/common.h>

typedef enum {
	IRC_ENABLE_INTERRUPT = IPC_FIRST_USER_METHOD,
	IRC_DISABLE_INTERRUPT,
	IRC_CLEAR_INTERRUPT
} irc_request_t;

#endif

/** @}
 */
