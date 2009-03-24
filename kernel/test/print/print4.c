/*
 * Copyright (c) 2009 Martin Decky
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
#include <test.h>

char *test_print4(bool quiet)
{
	if (!quiet) {
		printf("ASCII printable characters (32 - 127) using printf(\"%%c\") and printf(\"%%lc\"):\n");
		
		uint8_t hextet;
		for (hextet = 2; hextet < 8; hextet++) {
			printf("%#" PRIx8 ": ", hextet << 4);
			
			uint8_t index;
			for (index = 0; index < 16; index++)
				printf("%c", (char) ((hextet << 4) + index));
			
			printf("  ");
			for (index = 0; index < 16; index++)
				printf("%lc", (wchar_t) ((hextet << 4) + index));
			
			printf("\n");
		}
		
		printf("\nExtended ASCII characters (128 - 255) using printf(\"%%lc\"):\n");
		
		for (hextet = 8; hextet < 16; hextet++) {
			printf("%#" PRIx8 ": ", hextet << 4);
			
			uint8_t index;
			for (index = 0; index < 16; index++)
				printf("%lc", (wchar_t) ((hextet << 4) + index));
			
			printf("\n");
		}
		
		printf("\nUTF-8 strings using printf(\"%%s\"):\n");
		printf("English: %s\n", "Quick brown fox jumps over the lazy dog");
		printf("Czech:   %s\n", "Příliš žluťoučký kůň úpěl dábelské ódy");
		printf("Greek:   %s\n", "Ὦ ξεῖν’, ἀγγέλλειν Λακεδαιμονίοις ὅτι τῇδε");
		printf("Hebrew:  %s\n", "משוואת ברנולי היא משוואה בהידרודינמיקה");
		
		printf("\nUTF-32 strings using printf(\"%%ls\"):\n");
		printf("English: %ls\n", L"Quick brown fox jumps over the lazy dog");
		printf("Czech:   %ls\n", L"Příliš žluťoučký kůň úpěl dábelské ódy");
		printf("Greek:   %ls\n", L"Ὦ ξεῖν’, ἀγγέλλειν Λακεδαιμονίοις ὅτι τῇδε");
		printf("Hebrew:  %ls\n", L"משוואת ברנולי היא משוואה בהידרודינמיקה");
	}
	
	return NULL;
}
