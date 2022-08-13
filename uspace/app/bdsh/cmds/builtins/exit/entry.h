/*
 * SPDX-FileCopyrightText: 2022 HelenOS Project
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef EXIT_ENTRY_H_
#define EXIT_ENTRY_H_

#include "scli.h"

/* Entry points for the quit command */
extern void help_cmd_exit(unsigned int);
extern int cmd_exit(char *[], cliuser_t *);

#endif
