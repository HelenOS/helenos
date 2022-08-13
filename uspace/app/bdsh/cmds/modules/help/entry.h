/*
 * SPDX-FileCopyrightText: 2022 HelenOS Project
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef HELP_ENTRY_H_
#define HELP_ENTRY_H_

/* Entry points for the help command */
extern void help_cmd_help(unsigned int);
extern int cmd_help(char *[]);

#endif
