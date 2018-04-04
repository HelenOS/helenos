/*
 * Copyright (c) 2005 Jakub Jermar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup genericconsole
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
