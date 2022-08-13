/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file User interface
 */

#ifndef _UI_UI_H
#define _UI_UI_H

#include <display.h>
#include <errno.h>
#include <io/console.h>
#include <stdbool.h>
#include <types/ui/clickmatic.h>
#include <types/ui/ui.h>

extern errno_t ui_create(const char *, ui_t **);
extern errno_t ui_create_cons(console_ctrl_t *, ui_t **);
extern errno_t ui_create_disp(display_t *, ui_t **);
extern void ui_destroy(ui_t *);
extern void ui_quit(ui_t *);
extern void ui_run(ui_t *);
extern errno_t ui_paint(ui_t *);
extern bool ui_is_textmode(ui_t *);
extern bool ui_is_fullscreen(ui_t *);
extern errno_t ui_suspend(ui_t *);
extern errno_t ui_resume(ui_t *);
extern void ui_lock(ui_t *);
extern void ui_unlock(ui_t *);
extern ui_clickmatic_t *ui_get_clickmatic(ui_t *);

#endif

/** @}
 */
