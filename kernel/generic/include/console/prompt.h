/*
 * SPDX-FileCopyrightText: 2012 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_console
 * @{
 */
/** @file
 */

#ifndef KERN_CONSOLE_PROMPT_H_
#define KERN_CONSOLE_PROMPT_H_

#include <console/chardev.h>

#define MAX_TAB_HINTS 37

extern bool console_prompt_display_all_hints(indev_t *, size_t);
extern bool console_prompt_more_hints(indev_t *, size_t *);

#endif

/** @}
 */
