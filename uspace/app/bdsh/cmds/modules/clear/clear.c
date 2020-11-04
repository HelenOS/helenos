
#include "clear.h"
#include <stdio.h>
#include <stdlib.h>
#include "cmds.h"
#include "config.h"
#include "entry.h"
#include "errors.h"
#include "util.h"

static const char *cmdname = "clear";

/* Dispays help for clear in various levels */
void help_cmd_clear(unsigned int level)
{
	printf("This is the %s help for '%s'.\n", level ? EXT_HELP : SHORT_HELP,
	    cmdname);
	return;
}

/* Main entry point for clear, accepts an array of arguments */
int cmd_clear(char **argv)
{
	unsigned int argc;
	unsigned int i;

	printf("\033[H\033[J");
	fflush(stdout);

	/* Count the arguments */
	for (argc = 0; argv[argc] != NULL; argc++);

	// printf("%s %s\n", TEST_ANNOUNCE, cmdname);
	// printf("%d arguments passed to %s", argc - 1, cmdname);

	if (argc < 2) {
		printf("\n");
		printf("\033[H\033[J");
		fflush(stdout);
		return CMD_SUCCESS;
	}

	printf(":\n");
	for (i = 1; i < argc; i++)
		printf("[%d] -> %s\n", i, argv[i]);

	printf("\033[H\033[J");
	system("cls");
	fflush(stdout);

	return CMD_SUCCESS;
}
