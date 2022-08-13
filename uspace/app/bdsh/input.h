/*
 * SPDX-FileCopyrightText: 2008 Tim Post
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef INPUT_H
#define INPUT_H

#include "cmds/cmds.h"

/* prototypes */

extern void get_input(cliuser_t *);
extern errno_t process_input(cliuser_t *);
extern int input_init(void);

#endif
