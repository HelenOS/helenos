/*
 * SPDX-FileCopyrightText: 2008 Tim Post
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdlib.h>
#include "builtins.h"

#include "batch/entry.h"
#include "cd/entry.h"
#include "exit/entry.h"

builtin_t builtins[] = {
#include "batch/batch_def.inc"
#include "cd/cd_def.inc"
#include "exit/exit_def.inc"
	{ NULL, NULL, NULL, NULL, 0 }
};
