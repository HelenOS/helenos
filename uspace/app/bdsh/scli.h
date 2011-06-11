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

typedef struct {
	FILE *stdin;
	FILE *stdout;
	FILE *stderr;
} iostate_t;

extern const char *progname;

extern iostate_t *get_iostate(void);
extern void set_iostate(iostate_t *);

#endif
