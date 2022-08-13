/*
 * SPDX-FileCopyrightText: 2006 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_NS_H_
#define _LIBC_NS_H_

#include <ipc/services.h>
#include <task.h>
#include <async.h>

extern errno_t service_register(service_t, iface_t, async_port_handler_t,
    void *);
extern errno_t service_register_broker(service_t, async_port_handler_t, void *);
extern async_sess_t *service_connect(service_t, iface_t, sysarg_t, errno_t *);
extern async_sess_t *service_connect_blocking(service_t, iface_t, sysarg_t,
    errno_t *);

extern errno_t ns_ping(void);
extern errno_t ns_intro(task_id_t);
extern async_sess_t *ns_session_get(errno_t *);

#endif

/** @}
 */
