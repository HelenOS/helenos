/*
 * Copyright (c) 2012 Petr Koupy
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

/** @addtogroup vterm
 * @{
 */
/** @file
 */

#include <stdbool.h>
#include <stdio.h>
#include <io/pixel.h>
#include <task.h>
#include <window.h>
#include <terminal.h>

#define NAME  "vterm"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("%s: Compositor server not specified.\n", NAME);
		return 1;
	}

	window_t *main_window = window_open(argv[1], NULL,
	    WINDOW_MAIN | WINDOW_DECORATED, "vterm");
	if (!main_window) {
		printf("%s: Cannot open main window.\n", NAME);
		return 2;
	}

	window_resize(main_window, 0, 0, 648, 508, WINDOW_PLACEMENT_ANY);
	terminal_t *terminal_widget =
	    create_terminal(window_root(main_window), NULL, 640, 480);
	if (!terminal_widget) {
		window_close(main_window);
		printf("%s: Cannot create widgets.\n", NAME);
		return 3;
	}

	window_exec(main_window);
	task_retval(0);
	async_manager();

	return 0;
}

/** @}
 */
