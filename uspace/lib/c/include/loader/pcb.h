/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 * @brief Program Control Block interface.
 */

#ifndef _LIBC_PCB_H_
#define _LIBC_PCB_H_

#include <tls.h>

typedef void (*entry_point_t)(void);

struct pcb_inbox_entry {
	char *name;
	int file;
};

/** Program Control Block.
 *
 * Holds pointers to data passed from the program loader to the program
 * and/or to the dynamic linker. This includes the program entry point,
 * arguments, environment variables etc.
 *
 */
typedef struct {
	/** Program entry point. */
	entry_point_t entry;

	/** Current working directory. */
	char *cwd;

	/** Number of command-line arguments. */
	int argc;
	/** Command-line arguments. */
	char **argv;

	/** List of inbox files. */
	struct pcb_inbox_entry *inbox;
	int inbox_entries;

	/*
	 * ELF-specific data.
	 */

	/** Pointer to ELF dynamic section of the program. */
	void *dynamic;
	/** Pointer to dynamic linker state structure (rtld_t). */
	void *rtld_runtime;

	/** Thread local storage for the main thread. */
	tcb_t *tcb;
} pcb_t;

/**
 * A pointer to the program control block. Having received the PCB pointer,
 * the C library startup code stores it here for later use.
 */
extern pcb_t *__pcb;

#endif

/**
 * @}
 */
