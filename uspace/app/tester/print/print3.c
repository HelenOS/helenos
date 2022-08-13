/*
 * SPDX-FileCopyrightText: 2005 Josef Cejka
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stddef.h>
#include <macros.h>
#include "../tester.h"

#define BUFFER_SIZE  32

#ifndef __clang__
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif

const char *test_print3(void)
{
	char buffer[BUFFER_SIZE];
	int retval;

	TPRINTF("Testing snprintf(buffer, " STRING(BUFFER_SIZE) ", \"Short text without parameters.\"):\n");
	TPRINTF("Expected result: retval=30 buffer=\"Short text without parameters.\"\n");
	retval = snprintf(buffer, BUFFER_SIZE, "Short text without parameters.");
	TPRINTF("Real result:     retval=%d buffer=\"%s\"\n\n", retval, buffer);

	TPRINTF("Testing snprintf(buffer, " STRING(BUFFER_SIZE) ", \"Very very very long text without parameters.\"):\n");
	TPRINTF("Expected result: retval=44 buffer=\"Very very very long text withou\"\n");
	retval = snprintf(buffer, BUFFER_SIZE, "Very very very long text without parameters.");
	TPRINTF("Real result:     retval=%d buffer=\"%s\"\n\n", retval, buffer);

	TPRINTF("Testing snprintf(buffer, " STRING(BUFFER_SIZE) ", \"Short %%s.\", \"text\"):\n");
	TPRINTF("Expected result: retval=11 buffer=\"Short text.\"\n");
	retval = snprintf(buffer, BUFFER_SIZE, "Short %s.", "text");
	TPRINTF("Real result:     retval=%d buffer=\"%s\"\n\n", retval, buffer);

	TPRINTF("Testing snprintf(buffer, " STRING(BUFFER_SIZE) ", \"Very long %%s. This text's length is more than %%d. We are interested in the result.\", \"text\", " STRING(BUFFER_SIZE) "):\n");
	TPRINTF("Expected result: retval=84 buffer=\"Very long text. This text's len\"\n");
	retval = snprintf(buffer, BUFFER_SIZE, "Very long %s. This text's length is more than %d. We are interested in the result.", "text", BUFFER_SIZE);
	TPRINTF("Real result:     retval=%d buffer=\"%s\"\n\n", retval, buffer);

	return NULL;
}
