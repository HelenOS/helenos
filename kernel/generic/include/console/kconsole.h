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

#define MAX_CMDLINE     256
#define KCONSOLE_HISTORY 10

typedef enum {
	ARG_TYPE_INVALID = 0,
	ARG_TYPE_INT,
	ARG_TYPE_STRING,
	ARG_TYPE_VAR      /**< Variable type - either symbol or string */
} cmd_arg_type_t;

/** Structure representing one argument of kconsole command line. */
typedef struct {
	cmd_arg_type_t type;		/**< Type descriptor. */
	void *buffer;			/**< Buffer where to store data. */
	size_t len;			/**< Size of the buffer. */
	unative_t intval;                /**< Integer value */
	cmd_arg_type_t vartype;         /**< Resulting type of variable arg */
} cmd_arg_t;

/** Structure representing one kconsole command. */
typedef struct {
	link_t link;			/**< Command list link. */
	SPINLOCK_DECLARE(lock);		/**< This lock protects everything below. */
	const char *name;		/**< Command name. */
	const char *description;	/**< Textual description. */
	int (* func)(cmd_arg_t *);	/**< Function implementing the command. */
	count_t argc;			/**< Number of arguments. */
	cmd_arg_t *argv;		/**< Argument vector. */
	void (* help)(void);		/**< Function for printing detailed help. */
} cmd_info_t;

extern spinlock_t cmd_lock;
extern link_t cmd_head;

extern void kconsole_init(void);
extern void kconsole(void *prompt);

extern int cmd_register(cmd_info_t *cmd);

#endif

/** @}
 */
