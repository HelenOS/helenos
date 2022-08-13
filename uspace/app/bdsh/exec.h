/*
 * SPDX-FileCopyrightText: 2008 Tim Post
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef EXEC_H
#define EXEC_H

#include <task.h>
#include "scli.h"

extern const char *search_dir[];

extern unsigned int try_exec(char *, char **, iostate_t *);

#endif
