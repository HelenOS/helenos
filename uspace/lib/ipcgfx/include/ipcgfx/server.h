/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libipcgfx
 * @{
 */
/**
 * @file GFX IPC server
 */

#ifndef _IPCGFX_SERVER_H
#define _IPCGFX_SERVER_H

#include <async.h>
#include <errno.h>
#include <gfx/context.h>

extern errno_t gc_conn(ipc_call_t *icall, gfx_context_t *gc);

#endif

/** @}
 */
