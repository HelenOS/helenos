/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfx
 * @{
 */
/**
 * @file Graphics context
 */

#include <gfx/context.h>
#include <stdlib.h>
#include "../private/context.h"

/** Create new graphics context.
 *
 * Create new graphics context with the specified ops and argument.
 *
 * @param ops Graphics context ops
 * @param arg Instance argument
 * @param rgc Place to store pointer to new graphics context on success
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t gfx_context_new(gfx_context_ops_t *ops, void *arg,
    gfx_context_t **rgc)
{
	gfx_context_t *gc;

	gc = calloc(1, sizeof(gfx_context_t));
	if (gc == NULL)
		return ENOMEM;

	gc->ops = ops;
	gc->arg = arg;
	*rgc = gc;
	return EOK;
}

/** Delete graphics context.
 *
 * @param gc Graphics context or @c NULL
 */
errno_t gfx_context_delete(gfx_context_t *gc)
{
	if (gc == NULL)
		return EOK;

	free(gc);
	return EOK;
}

/** @}
 */
