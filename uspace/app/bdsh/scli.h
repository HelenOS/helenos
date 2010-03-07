#ifndef SCLI_H
#define SCLI_H

#include "config.h"
#include <stdint.h>

typedef struct {
	const char *name;
	char *line;
	char *cwd;
	char *prompt;
	int lasterr;
} cliuser_t;

#endif
