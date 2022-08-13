/*
 * SPDX-FileCopyrightText: 2017 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * @file
 * @brief Apple ADB keyboard controller
 */

#ifndef CTL_H
#define CTL_H

extern errno_t adb_kbd_key_translate(sysarg_t, kbd_event_type_t *, unsigned int *);

#endif

/** @}
 */
