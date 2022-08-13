/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
