#ifndef BUILTINS_H
#define BUILTINS_H

#include "config.h"

#include "batch/entry.h"
#include "cd/entry.h"
#include "exit/entry.h"

builtin_t builtins[] = {
#include "batch/batch_def.inc"
#include "cd/cd_def.inc"
#include "exit/exit_def.inc"
	{ NULL, NULL, NULL, NULL, 0 }
};

#endif
