#ifndef CD_ENTRY_H_
#define CD_ENTRY_H_

#include "scli.h"

/* Entry points for the cd command */
extern void help_cmd_cd(unsigned int);
extern int cmd_cd(char **, cliuser_t *);

#endif
