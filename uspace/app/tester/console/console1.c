/*
 * Copyright (c) 2008 Jiri Svoboda
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
#include <stdlib.h>
#include <io/console.h>
#include <io/color.h>
#include <io/style.h>
#include <vfs/vfs.h>
#include <async.h>
#include "../tester.h"

const char *color_name[] = {
	[COLOR_BLACK] = "black",
	[COLOR_BLUE] = "blue",
	[COLOR_GREEN] = "green",
	[COLOR_CYAN] = "cyan",
	[COLOR_RED] = "red",
	[COLOR_MAGENTA] = "magenta",
	[COLOR_YELLOW] = "yellow",
	[COLOR_WHITE] = "white"
};

char * test_console1(bool quiet)
{
	int i, j;

	printf("Style test: ");
	console_set_style(fphone(stdout), STYLE_NORMAL);
	printf("normal ");
	console_set_style(fphone(stdout), STYLE_EMPHASIS);
	printf("emphasized");
	console_set_style(fphone(stdout), STYLE_NORMAL);
	printf(".\n");

	printf("Foreground color test:\n");
	for (j = 0; j < 2; j++) {
		for (i = COLOR_BLACK; i <= COLOR_WHITE; i++) {
			console_set_color(fphone(stdout), i, COLOR_WHITE,
			    j ? CATTR_BRIGHT : 0);
			printf(" %s ", color_name[i]);
		}
		console_set_color(fphone(stdout), COLOR_BLACK, COLOR_WHITE, 0);
		putchar('\n');
	}

	printf("Background color test:\n");
	for (j = 0; j < 2; j++) {
		for (i = COLOR_BLACK; i <= COLOR_WHITE; i++) {
			console_set_color(fphone(stdout), COLOR_WHITE, i,
			    j ? CATTR_BRIGHT : 0);
			printf(" %s ", color_name[i]);
		}
		console_set_color(fphone(stdout), COLOR_BLACK, COLOR_WHITE, 0);
		putchar('\n');
	}

	printf("Now let's test RGB colors:\n");

	for (i = 0; i < 255; i += 16) {
		console_set_rgb_color(fphone(stdout), 0xffffff, i << 16);
		putchar('X');
	}
	console_set_color(fphone(stdout), COLOR_BLACK, COLOR_WHITE, 0);
	putchar('\n');

	for (i = 0; i < 255; i += 16) {
		console_set_rgb_color(fphone(stdout), 0xffffff, i << 8);
		putchar('X');
	}
	console_set_color(fphone(stdout), COLOR_BLACK, COLOR_WHITE, 0);
	putchar('\n');

	for (i = 0; i < 255; i += 16) {
		console_set_rgb_color(fphone(stdout), 0xffffff, i);
		putchar('X');
	}
	console_set_color(fphone(stdout), COLOR_BLACK, COLOR_WHITE, 0);
	putchar('\n');

	printf("[press a key]\n");
	getchar();

	return NULL;
}
