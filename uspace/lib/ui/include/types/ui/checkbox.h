/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Check box
 */

#ifndef _UI_TYPES_CHECKBOX_H
#define _UI_TYPES_CHECKBOX_H

#include <stdbool.h>

struct ui_checkbox;
typedef struct ui_checkbox ui_checkbox_t;

/** Check box callbacks */
typedef struct ui_checkbox_cb {
	void (*switched)(ui_checkbox_t *, void *, bool);
} ui_checkbox_cb_t;

#endif

/** @}
 */
