#ifndef BUILTINS_H
#define BUILTINS_H

#include "config.h"

#include "batch/entry.h"
#include "cd/entry.h"
#include "exit/entry.h"

builtin_t builtins[] = {
#include "batch/batch_def.h"
#include "cd/cd_def.h"
#include "exit/exit_def.h"
	{NULL, NULL, NULL, NULL, 0}
};

#endif
