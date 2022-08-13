/*
 * SPDX-FileCopyrightText: 2009 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup ns
 * @{
 */

#ifndef NS_SERVICE_H__
#define NS_SERVICE_H__

#include <ipc/common.h>
#include <ipc/services.h>
#include <abi/ipc/interfaces.h>

extern errno_t ns_service_init(void);
extern void ns_pending_conn_process(void);

extern errno_t ns_service_register(service_t, iface_t);
extern errno_t ns_service_register_broker(service_t);
extern void ns_service_forward(service_t, iface_t, ipc_call_t *);

#endif

/**
 * @}
 */
