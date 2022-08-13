/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libipcgfx
 * @{
 */
/**
 * @file GFX IPC backend
 */

#ifndef _IPCGFX_CLIENT_H
#define _IPCGFX_CLIENT_H

#include <async.h>
#include <types/ipcgfx/client.h>
#include <types/gfx/context.h>
#include <types/gfx/ops/context.h>

extern gfx_context_ops_t ipc_gc_ops;

extern errno_t ipc_gc_create(async_sess_t *, ipc_gc_t **);
extern errno_t ipc_gc_delete(ipc_gc_t *);
extern gfx_context_t *ipc_gc_get_ctx(ipc_gc_t *);

#endif

/** @}
 */
