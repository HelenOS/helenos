/*
 * SPDX-FileCopyrightText: 2009 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup ns
 * @{
 */

#ifndef NS_CLONABLE_H__
#define NS_CLONABLE_H__

#include <ipc/common.h>
#include <ipc/services.h>
#include <abi/ipc/interfaces.h>
#include <stdbool.h>

extern errno_t ns_clonable_init(void);

extern bool ns_service_is_clonable(service_t, iface_t);
extern void ns_clonable_register(ipc_call_t *);
extern void ns_clonable_forward(service_t, iface_t, ipc_call_t *);

#endif

/**
 * @}
 */
