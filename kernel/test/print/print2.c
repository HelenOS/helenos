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

const char *test_print2(void)
{
	TPRINTF("Testing printf(\"%%c\", 'a'):\n");
	TPRINTF("Expected output: [a]\n");
	TPRINTF("Real output:     [%c]\n\n", 'a');

	TPRINTF("Testing printf(\"%%d %%3.2d %%-3.2d %%2.3d %%-2.3d\", 1, 2, 3, 4, 5):\n");
	TPRINTF("Expected output: [1] [ 02] [03 ] [004] [005]\n");
	TPRINTF("Real output:     [%d] [%3.2d] [%-3.2d] [%2.3d] [%-2.3d]\n\n", 1, 2, 3, 4, 5);

	TPRINTF("Testing printf(\"%%d %%3.2d %%-3.2d %%2.3d %%-2.3d\", -1, -2, -3, -4, -5):\n");
	TPRINTF("Expected output: [-1] [-02] [-03] [-004] [-005]\n");
	TPRINTF("Real output:     [%d] [%3.2d] [%-3.2d] [%2.3d] [%-2.3d]\n\n", -1, -2, -3, -4, -5);

	TPRINTF("Testing printf(\"%%lld %%3.2lld %%-3.2lld %%2.3lld %%-2.3lld\", (long long) -1, (long long) -2, (long long) -3, (long long) -4, (long long) -5):\n");
	TPRINTF("Expected output: [-1] [-02] [-03] [-004] [-005]\n");
	TPRINTF("Real output:     [%lld] [%3.2lld] [%-3.2lld] [%2.3lld] [%-2.3lld]\n\n", (long long) -1, (long long) -2, (long long) -3, (long long) -4, (long long) -5);

	TPRINTF("Testing printf(\"%%#x %%5.3#x %%-5.3#x %%3.5#x %%-3.5#x\", 17, 18, 19, 20, 21):\n");
	TPRINTF("Expected output: [0x11] [0x012] [0x013] [0x00014] [0x00015]\n");
	TPRINTF("Real output:     [%#x] [%#5.3x] [%#-5.3x] [%#3.5x] [%#-3.5x]\n\n", 17, 18, 19, 20, 21);

	char ch[12];
	ptrdiff_t d, neg_d;

	d = &ch[0] - &ch[12];
	neg_d = (unsigned)(-d);
	TPRINTF("Testing printf(\"%%td %%tu %%tx %%ti %%to\", d, neg_d, neg_d, d, neg_d):\n");
	TPRINTF("Expected output: [-12] [12] [c] [-12] [14]\n");
	TPRINTF("Real output:     [%td] [%tu] [%tx] [%ti] [%to]\n\n", d, neg_d, neg_d, d, neg_d);

	sysarg_t nat = 0x12345678;

	TPRINTF("Testing printf(\"%%#" PRIx64 " %%#" PRIx32 " %%#" PRIx16 " %%#" PRIx8 " %%#" PRIxn " %%#" PRIx64 " %%s\", (uint64_t) UINT64_C(0x1234567887654321), (uint32_t) UINT32_C(0x12345678), (uint16_t) UINT16_C(0x1234), (uint8_t) UINT8_C(0x12), nat, (uint64_t) UINT64_C(0x1234567887654321), \"Lovely string\"):\n");
	TPRINTF("Expected output: [0x1234567887654321] [0x12345678] [0x1234] [0x12] [0x12345678] [0x1234567887654321] \"Lovely string\"\n");
	TPRINTF("Real output:     [%#" PRIx64 "] [%#" PRIx32 "] [%#" PRIx16 "] [%#" PRIx8 "] [%#" PRIxn "] [%#" PRIx64 "] \"%s\"\n\n", (uint64_t) UINT64_C(0x1234567887654321), (uint32_t) UINT32_C(0x12345678), (uint16_t) UINT16_C(0x1234), (uint8_t) UINT8_C(0x12), nat, (uint64_t) UINT64_C(0x1234567887654321), "Lovely string");

	return NULL;
}
