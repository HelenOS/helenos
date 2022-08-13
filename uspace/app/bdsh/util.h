/*
 * SPDX-FileCopyrightText: 2008 Tim Post
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UTIL_H
#define UTIL_H

#include "scli.h"
#include <stdbool.h>

/* Utility functions */
extern unsigned int cli_count_args(char **);
extern unsigned int cli_set_prompt(cliuser_t *usr);
extern bool is_path(const char *cmd);

#endif
