#ifndef EXEC_H
#define EXEC_H

#include <task.h>

extern char *find_command(char *);
extern unsigned int try_exec(char *, char **);
extern unsigned int try_access(const char *);

#endif
