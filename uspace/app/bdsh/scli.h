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
	uint64_t lasterr;
} cliuser_t;

extern unsigned int cli_set_prompt(cliuser_t *usr);

#endif
