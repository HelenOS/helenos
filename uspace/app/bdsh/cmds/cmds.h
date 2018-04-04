/*
 * Copyright (c) 2008 Tim Post
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
typedef int (*mod_entry_t)(char **);
typedef void (*mod_help_t)(unsigned int);

/* Built-in commands need to be able to modify cliuser_t */
typedef int (*builtin_entry_t)(char **, cliuser_t *);
typedef void (*builtin_help_t)(unsigned int);

/* Module structure */
typedef struct {
	const char *name;   /* Name of the command */
	const char *desc;   /* Description of the command */
	mod_entry_t entry;  /* Command (exec) entry function */
	mod_help_t help;    /* Command (help) entry function */
} module_t;

/* Builtin structure, same as modules except different types of entry points */
typedef struct {
	const char *name;
	const char *desc;
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
extern char *alias_for_module(const char *);
extern int help_module(int, unsigned int);
extern int run_module(int, char *[], iostate_t *);

/* Prototypes for builtin launchers */
extern int builtin_is_restricted(int);
extern int is_builtin(const char *);
extern int is_builtin_alias(const char *);
extern char *alias_for_builtin(const char *);
extern int help_builtin(int, unsigned int);
extern int run_builtin(int, char *[], cliuser_t *, iostate_t *);

#endif
