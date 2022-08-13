/*
 * SPDX-FileCopyrightText: 2008 Tim Post
 * SPDX-FileCopyrightText: 2018 Matthieu Riolo
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SCLI_H
#define SCLI_H

#include "config.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <types/adt/odict.h>

typedef struct {
	const char *name;
	char *line;
	char *cwd;
	char *prompt;
	errno_t lasterr;
} cliuser_t;

typedef struct {
	FILE *stdin;
	FILE *stdout;
	FILE *stderr;
} iostate_t;

extern const char *progname;

extern iostate_t *get_iostate(void);
extern void set_iostate(iostate_t *);

extern odict_t alias_dict;

typedef struct {
	odlink_t odict;
	char *name;
	char *value;
} alias_t;

#endif
