/*
 * SPDX-FileCopyrightText: 2010 Stanislav Kozina
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup top
 * @{
 */

#ifndef TOP_SCREEN_H_
#define TOP_SCREEN_H_

#include <errno.h>
#include <io/console.h>
#include <io/verify.h>
#include "top.h"

extern console_ctrl_t *console;

extern void screen_init(void);
extern void screen_done(void);
extern void print_data(data_t *);
extern void show_warning(const char *, ...)
    _HELENOS_PRINTF_ATTRIBUTE(1, 2);

extern errno_t tgetchar(sec_t, int *);

#endif

/**
 * @}
 */
