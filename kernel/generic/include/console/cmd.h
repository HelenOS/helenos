/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_console
 * @{
 */
/** @file
 */

#ifndef KERN_CMD_H_
#define KERN_CMD_H_

#include <console/kconsole.h>

extern void cmd_initialize(cmd_info_t *cmd);
extern void cmd_init(void);

#endif

/** @}
 */
