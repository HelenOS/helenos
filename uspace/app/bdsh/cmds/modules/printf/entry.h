/*
 * SPDX-FileCopyrightText: 2022 HelenOS Project
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PRINTF_ENTRY_H
#define PRINTF_ENTRY_H

/* Entry points for the printf command */
extern int cmd_printf(char **);
extern void help_cmd_printf(unsigned int);

#endif /* PRINTF_ENTRY_H */
