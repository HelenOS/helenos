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

#ifndef IPCP_H_
#define IPCP_H_

#include "proto.h"

extern void ipcp_init(void);
extern void ipcp_cleanup(void);

extern void ipcp_call_out(cap_phone_handle_t, ipc_call_t *, cap_call_handle_t);
extern void ipcp_call_sync(cap_phone_handle_t, ipc_call_t *, ipc_call_t *);
extern void ipcp_call_in(ipc_call_t *, cap_call_handle_t);
extern void ipcp_hangup(cap_phone_handle_t, errno_t);

extern void ipcp_connection_set(cap_phone_handle_t, int, proto_t *);
extern void ipcp_connection_clear(cap_phone_handle_t);

#endif

/** @}
 */
