/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_UDEBUG_IPC_H_
#define KERN_UDEBUG_IPC_H_

#include <ipc/ipc.h>

errno_t udebug_request_preprocess(call_t *call, phone_t *phone);
void udebug_call_receive(call_t *call);

#endif

/** @}
 */
