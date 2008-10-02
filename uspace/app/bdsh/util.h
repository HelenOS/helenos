#ifndef UTIL_H
#define UTIL_H

#include "scli.h"

/* Internal string handlers */
extern char * cli_strdup(const char *);
extern size_t cli_redup(char **, const char *);
extern int cli_psprintf(char **, const char *, ...);
extern char * cli_strtok_r(char *, const char *, char **);
extern char * cli_strtok(char *, const char *);

/* Utility functions */
extern unsigned int cli_count_args(char **);
extern unsigned int cli_set_prompt(cliuser_t *usr);

#endif
