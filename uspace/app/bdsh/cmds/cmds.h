#ifndef CMDS_H
#define CMDS_H

#include "config.h"
#include "scli.h"

/* Temporary to store strings */
#define EXT_HELP      "extended"
#define SHORT_HELP    "short"
#define TEST_ANNOUNCE "Hello, this is :"

/* Simple levels of help displays */
#define HELP_SHORT 0
#define HELP_LONG  1

/* Acceptable buffer sizes (for strn functions) */
/* TODO: Move me, other files duplicate these needlessly */
#define BUFF_LARGE  1024
#define BUFF_SMALL  255

/* Return macros for int type entry points */
#define CMD_FAILURE 1
#define CMD_SUCCESS 0

/* Types for module command entry and help */
typedef int (* mod_entry_t)(char **);
typedef void (* mod_help_t)(unsigned int);

/* Built-in commands need to be able to modify cliuser_t */
typedef int (* builtin_entry_t)(char **, cliuser_t *);
typedef void (* builtin_help_t)(unsigned int);

/* Module structure */
typedef struct {
	char *name;         /* Name of the command */
	char *desc;         /* Description of the command */
	mod_entry_t entry;  /* Command (exec) entry function */
	mod_help_t help;    /* Command (help) entry function */
} module_t;

/* Builtin structure, same as modules except different types of entry points */
typedef struct {
	char *name;
	char *desc;
	builtin_entry_t entry;
	builtin_help_t help;
	int restricted;
} builtin_t;

/* Declared in cmds/modules/modules.h and cmds/builtins/builtins.h
 * respectively */
extern module_t modules[];
extern builtin_t builtins[];

/* Prototypes for module launchers */
extern int module_is_restricted(int);
extern int is_module(const char *);
extern int is_module_alias(const char *);
extern char * alias_for_module(const char *);
extern int help_module(int, unsigned int);
extern int run_module(int, char *[]);

/* Prototypes for builtin launchers */
extern int builtin_is_restricted(int);
extern int is_builtin(const char *);
extern int is_builtin_alias(const char *);
extern char * alias_for_builtin(const char *);
extern int help_builtin(int, unsigned int);
extern int run_builtin(int, char *[], cliuser_t *);

#endif
