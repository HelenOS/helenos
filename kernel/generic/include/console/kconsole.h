/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_console
 * @{
 */
/** @file
 */

#ifndef KERN_KCONSOLE_H_
#define KERN_KCONSOLE_H_

#include <adt/list.h>
#include <synch/spinlock.h>
#include <ipc/irq.h>

#define MAX_CMDLINE       256
#define KCONSOLE_HISTORY  10

/** Callback to be used to enum hints for cmd tab completion. */
typedef const char *(*hints_enum_func_t)(const char *, const char **, void **);

typedef enum {
	ARG_TYPE_INVALID = 0,
	ARG_TYPE_INT,
	ARG_TYPE_STRING,
	/** Optional string */
	ARG_TYPE_STRING_OPTIONAL,
	/** Variable type - either symbol or string. */
	ARG_TYPE_VAR
} cmd_arg_type_t;

/** Structure representing one argument of kconsole command line. */
typedef struct {
	/** Type descriptor. */
	cmd_arg_type_t type;
	/** Buffer where to store data. */
	void *buffer;
	/** Size of the buffer. */
	size_t len;
	/** Integer value. */
	sysarg_t intval;
	/** Resulting type of variable arg */
	cmd_arg_type_t vartype;
} cmd_arg_t;

/** Structure representing one kconsole command. */
typedef struct {
	/** Command list link. */
	link_t link;
	/** This lock protects everything below. */
	SPINLOCK_DECLARE(lock);
	/** Command name. */
	const char *name;
	/** Textual description. */
	const char *description;
	/** Function implementing the command. */
	int (*func)(cmd_arg_t *);
	/** Number of arguments. */
	size_t argc;
	/** Argument vector. */
	cmd_arg_t *argv;
	/** Function for printing detailed help. */
	void (*help)(void);
	/** Function for enumerating hints for arguments. */
	hints_enum_func_t hints_enum;
} cmd_info_t;

extern bool kconsole_notify;
extern irq_t kconsole_irq;

SPINLOCK_EXTERN(cmd_lock);
extern list_t cmd_list;

extern void kconsole_init(void);
extern void kconsole_notify_init(void);
extern bool kconsole_check_poll(void);
extern void kconsole(const char *prompt, const char *msg, bool kcon);
extern void kconsole_thread(void *data);

extern bool cmd_register(cmd_info_t *cmd);
extern const char *cmdtab_enum(const char *name, const char **h, void **ctx);

#endif

/** @}
 */
