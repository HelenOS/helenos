#ifndef MODULES_H
#define MODULES_H

/* Each built in function has two files, one being an entry.h file which
 * prototypes the run/help entry functions, the other being a .def file
 * which fills the modules[] array according to the cmd_t structure
 * defined in cmds.h.
 *
 * To add or remove a module, just make a new directory in cmds/modules
 * for it and copy the 'show' example for basics, then include it here.
 * (or reverse the process to remove one)
 *
 * NOTE: See module_ aliases.h as well, this is where aliases (commands that
 * share an entry point with others) are indexed */

#include "config.h"

/* Prototypes for each module's entry (help/exec) points */

#include "help/entry.h"
#include "quit/entry.h"
#include "mkdir/entry.h"
#include "rm/entry.h"
#include "cat/entry.h"
#include "touch/entry.h"
#include "ls/entry.h"
#include "mount/entry.h"

/* Each .def function fills the module_t struct with the individual name, entry
 * point, help entry point, etc. You can use config.h to control what modules
 * are loaded based on what libraries exist on the system. */

module_t modules[] = {
#include "help/help.def"
#include "quit/quit.def"
#include "mkdir/mkdir.def"
#include "rm/rm.def"
#include "cat/cat.def"
#include "touch/touch.def"
#include "ls/ls.def"
#include "mount/mount.def"
	{NULL, NULL, NULL, NULL}
};

#endif
