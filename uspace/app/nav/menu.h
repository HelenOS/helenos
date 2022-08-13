/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup nav
 * @{
 */
/**
 * @file Navigator menu
 */

#ifndef MENU_H
#define MENU_H

#include <errno.h>
#include <ui/menuentry.h>
#include <ui/window.h>
#include "types/menu.h"

extern errno_t nav_menu_create(ui_window_t *, nav_menu_t **);
extern void nav_menu_set_cb(nav_menu_t *, nav_menu_cb_t *, void *);
extern void nav_menu_destroy(nav_menu_t *);
extern ui_control_t *nav_menu_ctl(nav_menu_t *);
extern void nav_menu_file_open(ui_menu_entry_t *, void *);
extern void nav_menu_file_exit(ui_menu_entry_t *, void *);

#endif

/** @}
 */
