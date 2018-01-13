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

/*
 * This test tests several features of the HelenOS
 * printf() implementation which go beyond the POSIX
 * specification and GNU printf() behaviour.
 *
 * Therefore we disable printf() argument checking by
 * the GCC compiler in this source file to avoid false
 * positives.
 *
 */
#define _HELENOS_NVERIFY_PRINTF

#include <stdio.h>
#include <stddef.h>
#include "../tester.h"

const char *test_print5(void)
{
	TPRINTF("Testing printf(\"%%s\", NULL):\n");
	TPRINTF("Expected output: \"(NULL)\"\n");
	TPRINTF("Real output:     \"%s\"\n\n", (char *) NULL);
	
	TPRINTF("Testing printf(\"%%c %%3.2c %%-3.2c %%2.3c %%-2.3c\", 'a', 'b', 'c', 'd', 'e'):\n");
	TPRINTF("Expected output: [a] [  b] [c  ] [ d] [e ]\n");
	TPRINTF("Real output:     [%c] [%3.2c] [%-3.2c] [%2.3c] [%-2.3c]\n\n", 'a', 'b', 'c', 'd', 'e');
	
	return NULL;
}
