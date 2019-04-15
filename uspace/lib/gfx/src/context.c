/*
 * Copyright (c) 2019 Jiri Svoboda
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
