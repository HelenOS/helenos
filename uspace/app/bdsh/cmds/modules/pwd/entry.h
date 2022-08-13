/*
 * SPDX-FileCopyrightText: 2022 HelenOS Project
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PWD_ENTRY_H
#define PWD_ENTRY_H

#include "scli.h"

/* Entry points for the pwd command */
extern void help_cmd_pwd(unsigned int);
extern int cmd_pwd(char **);

#endif
