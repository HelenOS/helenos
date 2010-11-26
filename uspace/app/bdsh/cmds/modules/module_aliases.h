#ifndef MODULE_ALIASES_H
#define MODULE_ALIASES_H

/* Modules that declare multiple names for themselves but use the
 * same entry functions are aliases. This array helps to determine if
 * a module is an alias, as such it can be invoked differently.
 * format is alias , real_name */

/* So far, this is only used in the help display but could be used to
 * handle a module differently even prior to reaching its entry code.
 * For instance, 'exit' could behave differently than 'quit', prior to
 * the entry point being reached. */

const char *mod_aliases[] = {
	"ren", "mv",
	"umount", "unmount",
	NULL, NULL
};

#endif
