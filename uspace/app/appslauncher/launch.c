/*
 * Copyright (c) 2023 SimonJRiddix
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
#include <str.h>
#include <str_error.h>
#include <task.h>
#include <ui/ui.h>

static const char *display_spec = UI_DISPLAY_DEFAULT;
int app_launchl(const char *app, ...);

#define NAME  "Apps Menu"

int app_launchl(const char *app, ...)
{
	errno_t rc;
	task_id_t id;
	task_wait_t wait;
	va_list ap;
	const char *arg;
	const char **argv;
	const char **argp;
	int cnt = 0;
	int i;

	va_start(ap, app);
	do {
		arg = va_arg(ap, const char *);
		cnt++;
	} while (arg != NULL);
	va_end(ap);

	argv = calloc(cnt + 4, sizeof(const char *));
	if (argv == NULL)
		return -1;

	task_exit_t texit;
	int retval;

	argp = argv;
	*argp++ = app;

	if (str_cmp(display_spec, UI_DISPLAY_DEFAULT) != 0) {
		*argp++ = "-d";
		*argp++ = display_spec;
	}

	va_start(ap, app);
	do {
		arg = va_arg(ap, const char *);
		*argp++ = arg;
	} while (arg != NULL);
	va_end(ap);

	*argp++ = NULL;

	printf("%s: Spawning %s", NAME, app);
	for (i = 0; argv[i] != NULL; i++) {
		printf(" %s", argv[i]);
	}
	printf("\n");

	rc = task_spawnv(&id, &wait, app, argv);
	if (rc != EOK) {
		printf("%s: Error spawning %s (%s)\n", NAME, app, str_error(rc));
		return -1;
	}

	rc = task_wait(&wait, &texit, &retval);
	if ((rc != EOK) || (texit != TASK_EXIT_NORMAL)) {
		printf("%s: Error retrieving retval from %s (%s)\n", NAME,
		    app, str_error(rc));
		return -1;
	}

	return retval;
}
