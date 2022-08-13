/*
 * SPDX-FileCopyrightText: 2009 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <test.h>
#include <typedefs.h>

const char *test_print4(void)
{
	TPRINTF("ASCII printable characters (32 - 127) using printf(\"%%c\") and printf(\"%%lc\"):\n");

	uint8_t group;
	for (group = 1; group < 4; group++) {
		TPRINTF("%#x: ", group << 5);

		uint8_t index;
		for (index = 0; index < 32; index++)
			TPRINTF("%c", (char) ((group << 5) + index));

		TPRINTF("  ");
		for (index = 0; index < 32; index++)
			TPRINTF("%lc", (wint_t) ((group << 5) + index));

		TPRINTF("\n");
	}

	TPRINTF("\nExtended ASCII characters (128 - 255) using printf(\"%%lc\"):\n");

	for (group = 4; group < 8; group++) {
		TPRINTF("%#x: ", group << 5);

		uint8_t index;
		for (index = 0; index < 32; index++)
			TPRINTF("%lc", (wint_t) ((group << 5) + index));

		TPRINTF("\n");
	}

	TPRINTF("\nUTF-8 strings using printf(\"%%s\"):\n");
	TPRINTF("English:  %s\n", "Quick brown fox jumps over the lazy dog");
	TPRINTF("Czech:    %s\n", "Příliš žluťoučký kůň úpěl ďábelské ódy");
	TPRINTF("Greek:    %s\n", "Ὦ ξεῖν’, ἀγγέλλειν Λακεδαιμονίοις ὅτι τῇδε");
	TPRINTF("Hebrew:   %s\n", "משוואת ברנולי היא משוואה בהידרודינמיקה");
	TPRINTF("Arabic:   %s\n", "التوزيع الجغرافي للحمل العنقودي");
	TPRINTF("Russian:  %s\n", "Леннон познакомился с художницей-авангардисткой");
	TPRINTF("Armenian: %s\n", "Սկսեց հրատարակվել Երուսաղեմի հայկական");

	TPRINTF("\nUTF-32 strings using printf(\"%%ls\"):\n");
	TPRINTF("English:  %ls\n", L"Quick brown fox jumps over the lazy dog");
	TPRINTF("Czech:    %ls\n", L"Příliš žluťoučký kůň úpěl ďábelské ódy");
	TPRINTF("Greek:    %ls\n", L"Ὦ ξεῖν’, ἀγγέλλειν Λακεδαιμονίοις ὅτι τῇδε");
	TPRINTF("Hebrew:   %ls\n", L"משוואת ברנולי היא משוואה בהידרודינמיקה");
	TPRINTF("Arabic:   %ls\n", L"التوزيع الجغرافي للحمل العنقودي");
	TPRINTF("Russian:  %ls\n", L"Леннон познакомился с художницей-авангардисткой");
	TPRINTF("Armenian: %ls\n", L"Սկսեց հրատարակվել Երուսաղեմի հայկական");

	return NULL;
}
