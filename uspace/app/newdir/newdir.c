/*
 * Copyright (c) 2026 Jiri Svoboda
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

/** @addtogroup newdir
 * @{
 */
/** @file Create new directory.
 */

#include <capa.h>
#include <errno.h>
#include <fmgt.h>
#include <io/console.h>
#include <io/cons_event.h>
#include <io/kbd_event.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>

#define NAME  "newdir"

static fmgt_error_action_t newdir_io_error_query(void *, fmgt_io_error_t *);

static bool nonint = false;
static bool quiet = false;

static console_ctrl_t *con;

static fmgt_cb_t newdir_fmgt_cb = {
	.io_error_query = newdir_io_error_query
};

static void print_syntax(void)
{
	printf("Create new directory.\n");
	printf("Syntax: %s [<options] [<directory-name>]\n", NAME);
	printf("\t-h           help\n");
	printf("\t-n           non-interactive\n");
	printf("\t-q           quiet\n");
}

/** Called by fmgt to let user choose I/O error recovery action.
 *
 * @param arg Argument (not used)
 * @param err I/O error report
 * @return Error recovery action.
 */
static fmgt_error_action_t newdir_io_error_query(void *arg,
    fmgt_io_error_t *err)
{
	cons_event_t event;
	kbd_event_t *ev;
	errno_t rc;

	(void)arg;

	if (nonint)
		return fmgt_er_abort;

	fprintf(stderr, "I/O error creating directory '%s' (%s).\n",
	    err->fname, str_error(err->rc));
	fprintf(stderr, "[A]bort or [R]etry?\n");

	if (con == NULL)
		return fmgt_er_abort;

	while (true) {
		rc = console_get_event(con, &event);
		if (rc != EOK)
			return fmgt_er_abort;

		if (event.type == CEV_KEY && event.ev.key.type == KEY_PRESS) {
			ev = &event.ev.key;
			if ((ev->mods & KM_ALT) == 0 &&
			    (ev->mods & KM_CTRL) == 0) {
				if (ev->c == 'r' || ev->c == 'R')
					return fmgt_er_retry;
				if (ev->c == 'a' || ev->c == 'A')
					return fmgt_er_abort;
			}
		}

		if (event.type == CEV_KEY && event.ev.key.type == KEY_PRESS) {
			ev = &event.ev.key;
			if ((ev->mods & KM_ALT) == 0 &&
			    (ev->mods & KM_SHIFT) == 0 &&
			    (ev->mods & KM_CTRL) != 0) {
				if (ev->key == KC_C)
					return fmgt_er_abort;
			}
		}
	}
}

int main(int argc, char *argv[])
{
	fmgt_t *fmgt = NULL;
	errno_t rc;
	int i;
	char *dname = NULL;

	con = console_init(stdin, stdout);

	i = 1;
	while (i < argc && argv[i][0] == '-') {
		if (str_cmp(argv[i], "-h") == 0) {
			print_syntax();
			return 0;
		} else if (str_cmp(argv[i], "-n") == 0) {
			++i;
			nonint = true;
		} else if (str_cmp(argv[i], "-q") == 0) {
			++i;
			quiet = true;
		} else {
			printf("Invalid option '%s'.\n", argv[i]);
			print_syntax();
			goto error;
		}
	}

	if (i < argc) {
		dname = str_dup(argv[i++]);
		if (dname == NULL) {
			printf("Out of memory.\n");
			goto error;
		}
	}

	if (i < argc) {
		printf("Unexpected argument.\n");
		print_syntax();
		goto error;
	}

	if (dname == NULL) {
		rc = fmgt_new_dir_suggest(&dname);
		if (rc != EOK) {
			printf("Out of memory.\n");
			goto error;
		}
	}

	rc = fmgt_create(&fmgt);
	if (rc != EOK) {
		printf("Out of memory.\n");
		goto error;
	}

	fmgt_set_cb(fmgt, &newdir_fmgt_cb, NULL);

	rc = fmgt_new_dir(fmgt, dname);
	if (rc != EOK) {
		printf("Error creating directory: %s.\n", str_error(rc));
		goto error;
	}

	free(dname);
	fmgt_destroy(fmgt);
	return 0;
error:
	if (dname != NULL)
		free(dname);
	if (fmgt != NULL)
		fmgt_destroy(fmgt);
	return 1;
}

/** @}
 */
