/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Menu entry
 */

#ifndef _UI_TYPES_MENUENTRY_H
#define _UI_TYPES_MENUENTRY_H

struct ui_menu_entry;
typedef struct ui_menu_entry ui_menu_entry_t;

/** Menu entry callback */
typedef void (*ui_menu_entry_cb_t)(ui_menu_entry_t *, void *);

#endif

/** @}
 */
