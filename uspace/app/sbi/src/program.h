/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PROGRAM_H_
#define PROGRAM_H_

#include "mytypes.h"

errno_t program_file_process(stree_program_t *program, const char *fname);
errno_t program_lib_process(stree_program_t *program);

#endif
