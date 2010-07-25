#ifndef BUILTINS_H
#define BUILTINS_H

#include "config.h"

#include "cd/entry.h"
#include "exit/entry.h"

builtin_t builtins[] = {
#include "cd/cd_def.h"
#include "exit/exit_def.h"
	{NULL, NULL, NULL, NULL, NULL}
};

#endif
