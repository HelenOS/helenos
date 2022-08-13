/*
 * SPDX-FileCopyrightText: 2008 Tim Post
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*
 * NOTES:
 * module_* functions are pretty much identical to builtin_* functions at this
 * point. On the surface, it would appear that making each function dual purpose
 * would be economical.
 *
 * These are kept separate because the structures (module_t and builtin_t) may
 * grow apart and become rather different, even though they're identical at this
 * point.
 *
 * To keep things easy to hack, everything is separated. In reality this only adds
 * 6 - 8 extra functions, but keeps each function very easy to read and modify.
 */

/*
 * TODO:
 * Many of these could be unsigned, provided the modules and builtins themselves
 * can follow suit. Long term goal.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <str.h>
#include "errors.h"
#include "cmds.h"
#include "module_aliases.h"

extern volatile unsigned int cli_interactive;

/** Checks if an entry function matching command exists in modules[]
 *
 * If so, its position in the array is returned
 */
int is_module(const char *command)
{
	module_t *mod;
	unsigned int i = 0;

	if (NULL == command)
		return -2;

	for (mod = modules; mod->name != NULL; mod++, i++) {
		if (!str_cmp(mod->name, command))
			return i;
	}

	return -1;
}

/*
 * Checks if a module is an alias (sharing an entry point with another
 * module). Returns 1 if so
 */
int is_module_alias(const char *command)
{
	unsigned int i = 0;

	if (NULL == command)
		return -1;

	for (i = 0; mod_aliases[i] != NULL; i += 2) {
		if (!str_cmp(mod_aliases[i], command))
			return 1;
	}

	return 0;
}

/* Returns the name of the module that an alias points to */
char *alias_for_module(const char *command)
{
	unsigned int i = 0;

	if (NULL == command)
		return (char *)NULL;

	for (i = 0; mod_aliases[i] != NULL; i += 2) {
		if (!str_cmp(mod_aliases[i], command))
			return (char *)mod_aliases[i + 1];
	}

	return (char *)NULL;
}

/** Invokes the 'help' entry function for the module at position (int) module
 *
 * which wants an unsigned int to determine brief or extended display.
 */
int help_module(int module, unsigned int extended)
{
	module_t *mod = modules;

	mod += module;

	if (NULL != mod->help) {
		mod->help(extended);
		return CL_EOK;
	} else {
		return CL_ENOENT;
	}
}

/** Invokes the module entry point modules[module]
 *
 * passing argv[] as an argument stack.
 */
int run_module(int module, char *argv[], iostate_t *new_iostate)
{
	int rc;
	module_t *mod = modules;

	mod += module;

	iostate_t *old_iostate = get_iostate();
	set_iostate(new_iostate);

	if (NULL != mod->entry) {
		rc = ((int)mod->entry(argv));
	} else {
		rc = CL_ENOENT;
	}

	set_iostate(old_iostate);

	return rc;
}
