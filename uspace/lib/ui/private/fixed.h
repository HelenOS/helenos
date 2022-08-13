/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Fixed layout structure
 *
 */

#ifndef _UI_PRIVATE_FIXED_H
#define _UI_PRIVATE_FIXED_H

#include <adt/list.h>
#include <gfx/coord.h>
#include <stdbool.h>
#include <types/ui/fixed.h>

/** Actual structure of fixed layout.
 *
 * This is private to libui.
 */
struct ui_fixed {
	/** Base control object */
	struct ui_control *control;
	/** Layout elements (ui_fixed_elem_t) */
	list_t elem;
};

/** Fixed layout element. */
typedef struct {
	/** Containing fixed layout */
	struct ui_fixed *fixed;
	/** Link to @c fixed->elem list */
	link_t lelems;
	/** Control */
	ui_control_t *control;
} ui_fixed_elem_t;

extern ui_fixed_elem_t *ui_fixed_first(ui_fixed_t *f);
extern ui_fixed_elem_t *ui_fixed_next(ui_fixed_elem_t *);

#endif

/** @}
 */
