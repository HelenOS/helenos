/*
 * SPDX-FileCopyrightText: 2022 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libui
 * @{
 */
/**
 * @file Accelerator processing
 */

#ifndef _UI_ACCEL_H
#define _UI_ACCEL_H

#include <errno.h>
#include <uchar.h>

extern errno_t ui_accel_process(const char *, char **, char **);
extern char32_t ui_accel_get(const char *);

#endif

/** @}
 */
