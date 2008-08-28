#ifndef BUILTINS_H
#define BUILTINS_H

#include "config.h"

#include "cd/entry.h"

builtin_t builtins[] = {
#include "cd/cd.def"
	{NULL, NULL, NULL, NULL}
};

#endif
