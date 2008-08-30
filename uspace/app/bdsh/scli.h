#ifndef SCLI_H
#define SCLI_H

#include "config.h"
#include <stdint.h>

typedef struct {
	char *name;
	char *home;
	char *line;
	char *cwd;
	char *prompt;
	int lasterr;
} cliuser_t;

#endif
