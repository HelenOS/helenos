/*
 * SPDX-FileCopyrightText: 2022 HelenOS Project
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CD_ENTRY_H_
#define CD_ENTRY_H_

#include "scli.h"

/* Entry points for the cd command */
extern void help_cmd_cd(unsigned int);
extern int cmd_cd(char **, cliuser_t *);

#endif
