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

#include <print.h>
#include <stddef.h>
#include <test.h>

const char *test_print1(void)
{
	TPRINTF("Testing printf(\"%%*.*s\", 5, 3, \"text\"):\n");
	TPRINTF("Expected output: \"  tex\"\n");
	TPRINTF("Real output:     \"%*.*s\"\n\n", 5, 3, "text");

	TPRINTF("Testing printf(\"%%10.8s\", \"very long text\"):\n");
	TPRINTF("Expected output: \"  very lon\"\n");
	TPRINTF("Real output:     \"%10.8s\"\n\n", "very long text");

	TPRINTF("Testing printf(\"%%8.10s\", \"text\"):\n");
	TPRINTF("Expected output: \"    text\"\n");
	TPRINTF("Real output:     \"%8.10s\"\n\n", "text");

	TPRINTF("Testing printf(\"%%8.10s\", \"very long text\"):\n");
	TPRINTF("Expected output: \"very long \"\n");
	TPRINTF("Real output:     \"%8.10s\"\n\n", "very long text");

	TPRINTF("Testing printf(\"%%-*.*s\", 7, 7, \"text\"):\n");
	TPRINTF("Expected output: \"text   \"\n");
	TPRINTF("Real output:     \"%-*.*s\"\n\n", 7, 7, "text");

	return NULL;
}
