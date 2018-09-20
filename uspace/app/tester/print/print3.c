/*
 * Copyright (c) 2005 Josef Cejka
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
