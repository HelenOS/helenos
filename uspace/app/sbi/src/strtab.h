/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef STRTAB_H_
#define STRTAB_H_

#include "mytypes.h"

void strtab_init(void);
sid_t strtab_get_sid(const char *str);
char *strtab_get_str(sid_t sid);

#endif
