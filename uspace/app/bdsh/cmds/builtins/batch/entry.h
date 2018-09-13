#ifndef BATCH_ENTRY_H
#define BATCH_ENTRY_H

/* Pick up cliuser_t */
#include "scli.h"

/* Entry points for the batch command */
extern int cmd_batch(char **, cliuser_t *);
extern void help_cmd_batch(unsigned int);

#endif /* BATCH_ENTRY_H */
