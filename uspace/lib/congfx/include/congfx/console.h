/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcongfx
 * @{
 */
/**
 * @file GFX console backend
 */

#ifndef _CONGFX_CONSOLE_H
#define _CONGFX_CONSOLE_H

#include <io/console.h>
#include <stdio.h>
#include <types/congfx/console.h>
#include <types/gfx/context.h>
#include <types/gfx/ops/context.h>

extern gfx_context_ops_t console_gc_ops;

extern errno_t console_gc_create(console_ctrl_t *, FILE *, console_gc_t **);
extern errno_t console_gc_delete(console_gc_t *);
extern errno_t console_gc_suspend(console_gc_t *);
extern errno_t console_gc_resume(console_gc_t *);
extern gfx_context_t *console_gc_get_ctx(console_gc_t *);

#endif

/** @}
 */
