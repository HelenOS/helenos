/*
 * SPDX-FileCopyrightText: 2010 Lenka Trochtova
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdrv
 * @{
 */
/** @file
 */

#ifndef LIBDRV_DEV_IFACE_H_
#define LIBDRV_DEV_IFACE_H_

#include <ipc/common.h>
#include <ipc/dev_iface.h>
#include <stdbool.h>

/*
 * Device interface
 */

struct ddf_fun;

/*
 * First two parameters: device and interface structure registered by the
 * devices driver.
 */
typedef void remote_iface_func_t(struct ddf_fun *, void *, ipc_call_t *);
typedef remote_iface_func_t *remote_iface_func_ptr_t;
typedef void remote_handler_t(struct ddf_fun *, ipc_call_t *);

typedef struct {
	const size_t method_count;
	const remote_iface_func_ptr_t *methods;
} remote_iface_t;

typedef struct {
	const remote_iface_t *ifaces[DEV_IFACE_COUNT];
} iface_dipatch_table_t;

extern const remote_iface_t *get_remote_iface(int);
extern remote_iface_func_ptr_t get_remote_method(const remote_iface_t *, sysarg_t);

extern bool is_valid_iface_idx(int);

#endif

/**
 * @}
 */
