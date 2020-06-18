/*
 * Copyright (c) 2011 Jiri Svoboda
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

/**
 * @addtogroup inputgen generic
 * @brief Keyboard layouts
 * @ingroup input
 * @{
 */
/** @file
 */

#include <errno.h>
#include <stdlib.h>
#include "input.h"
#include "layout.h"

/** Create a new layout instance. */
layout_t *layout_create(layout_ops_t *ops)
{
	layout_t *layout;

	layout = calloc(1, sizeof(layout_t));
	if (layout == NULL) {
		printf("%s: Out of memory.\n", NAME);
		return NULL;
	}

	layout->ops = ops;
	if ((*ops->create)(layout) != EOK) {
		free(layout);
		return NULL;
	}

	return layout;
}

/** Destroy layout instance. */
void layout_destroy(layout_t *layout)
{
	(*layout->ops->destroy)(layout);
	free(layout);
}

/** Parse keyboard event. */
char32_t layout_parse_ev(layout_t *layout, kbd_event_t *ev)
{
	return (*layout->ops->parse_ev)(layout, ev);
}

/**
 * @}
 */
