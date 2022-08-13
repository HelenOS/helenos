/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup nav
 * @{
 */
/**
 * @file Navigator
 */

#ifndef NAV_H
#define NAV_H

#include <errno.h>
#include "types/nav.h"
#include "types/panel.h"

extern errno_t navigator_create(const char *, navigator_t **);
extern void navigator_destroy(navigator_t *);
extern errno_t navigator_run(const char *);
extern panel_t *navigator_get_active_panel(navigator_t *);
extern void navigator_switch_panel(navigator_t *);

#endif

/** @}
 */
