/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef INPUT_T_H_
#define INPUT_T_H_

#include <stdio.h>

/** Input state object */
typedef struct input {
	/** Input name (for error output) */
	const char *name;

	/** Input file if reading from file. */
	FILE *fin;

	/** Input string if reading from string. */
	const char *str;

	/** Buffer holding current line. */
	char *buffer;

	/** Current line number */
	int line_no;
} input_t;

#endif
