#ifndef INPUT_H
#define INPUT_H

#include "cmds/cmds.h"

/* prototypes */
extern int tok_input(cliuser_t *);
extern void get_input(cliuser_t *);
extern void cli_restricted(char *);
extern void read_line(char *, int);

#endif
