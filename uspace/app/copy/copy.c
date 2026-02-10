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

/** @addtogroup copy
 * @{
 */
/** @file Copy files and directories.
 */

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

#define NAME  "copy"

static bool copy_abort_query(void *);
static void copy_progress(void *, fmgt_progress_t *);
static fmgt_error_action_t copy_io_error_query(void *, fmgt_io_error_t *);
static fmgt_exists_action_t copy_exists_query(void *, fmgt_exists_t *);

static bool prog_upd = false;
static bool nonint = false;
static bool quiet = false;

static console_ctrl_t *con;

static fmgt_cb_t copy_fmgt_cb = {
	.abort_query = copy_abort_query,
	.io_error_query = copy_io_error_query,
	.exists_query = copy_exists_query,
	.progress = copy_progress,
};

static void print_syntax(void)
{
	printf("Copy files and directories.\n");
	printf("Syntax: %s [<options] <source>... <dest>\n", NAME);
	printf("\t-h    help\n");
	printf("\t-n    non-interactive\n");
	printf("\t-q    quiet\n");
}

/** Called by fmgt to query for user abort.
 *
 * @param arg Argument (not used)
 * @return @c true iff user requested abort
 */
static bool copy_abort_query(void *arg)
{
	cons_event_t event;
	kbd_event_t *ev;
	errno_t rc;
	usec_t timeout;

	if (con == NULL)
		return false;

	timeout = 0;
	rc = console_get_event_timeout(con, &event, &timeout);
	if (rc != EOK)
		return false;

	if (event.type == CEV_KEY && event.ev.key.type == KEY_PRESS) {
		ev = &event.ev.key;
		if ((ev->mods & KM_ALT) == 0 &&
		    (ev->mods & KM_SHIFT) == 0 &&
		    (ev->mods & KM_CTRL) != 0) {
			if (ev->key == KC_C)
				return true;
		}
	}

	return false;
}

/** Called by fmgt to give the user progress update.
 *
 * @param arg Argument (not used)
 * @param progress Progress report
 */
static void copy_progress(void *arg, fmgt_progress_t *progress)
{
	(void)arg;

	if (!quiet) {
		printf("\rCopied %s files, %s; current file: %s done.  "
		    "\b\b", progress->total_procf, progress->total_procb,
		    progress->curf_percent);
		fflush(stdout);
		prog_upd = true;
	}
}

/** Called by fmgt to let user choose I/O error recovery action.
 *
 * @param arg Argument (not used)
 * @param err I/O error report
 * @return Error recovery action.
 */
static fmgt_error_action_t copy_io_error_query(void *arg,
    fmgt_io_error_t *err)
{
	cons_event_t event;
	kbd_event_t *ev;
	const char *fmt = NULL;
	errno_t rc;

	(void)arg;

	if (nonint)
		return fmgt_er_abort;

	if (prog_upd)
		putchar('\n');

	switch (err->optype) {
	case fmgt_io_read:
		fmt = "Error reading file %s. (%s)\n";
		break;
	case fmgt_io_write:
		fmt = "Error writing file %s. (%s)\n";
		break;
	case fmgt_io_create:
		fmt = "Error creating %s. (%s)\n";
		break;
	case fmgt_io_open:
		fmt = "Error opening %s. (%s)\n";
		break;
	}

	fprintf(stderr, fmt, err->fname, str_error(err->rc));
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

/** Called by fmgt to let user choose destination exists recovery action.
 *
 * @param arg Argument (not used)
 * @param err I/O error report
 * @return Error recovery action.
 */
static fmgt_exists_action_t copy_exists_query(void *arg, fmgt_exists_t *exists)
{
	cons_event_t event;
	kbd_event_t *ev;
	errno_t rc;

	(void)arg;

	if (nonint)
		return fmgt_exr_abort;

	if (prog_upd)
		putchar('\n');

	fprintf(stderr, "File %s exists.\n", exists->fname);
	fprintf(stderr, "[O]verwrite, [S]kip or [A]bort?\n");

	if (con == NULL)
		return fmgt_exr_abort;

	while (true) {
		rc = console_get_event(con, &event);
		if (rc != EOK)
			return fmgt_exr_abort;

		if (event.type == CEV_KEY && event.ev.key.type == KEY_PRESS) {
			ev = &event.ev.key;
			if ((ev->mods & KM_ALT) == 0 &&
			    (ev->mods & KM_CTRL) == 0) {
				if (ev->c == 'o' || ev->c == 'O')
					return fmgt_exr_overwrite;
				if (ev->c == 's' || ev->c == 'S')
					return fmgt_exr_skip;
				if (ev->c == 'a' || ev->c == 'A')
					return fmgt_exr_abort;
			}
		}

		if (event.type == CEV_KEY && event.ev.key.type == KEY_PRESS) {
			ev = &event.ev.key;
			if ((ev->mods & KM_ALT) == 0 &&
			    (ev->mods & KM_SHIFT) == 0 &&
			    (ev->mods & KM_CTRL) != 0) {
				if (ev->key == KC_C)
					return fmgt_exr_abort;
			}
		}
	}
}

int main(int argc, char *argv[])
{
	fmgt_t *fmgt = NULL;
	errno_t rc;
	int i;
	fmgt_flist_t *flist = NULL;
	const char *dest;

	rc = fmgt_flist_create(&flist);
	if (rc != EOK) {
		printf("Out of memory.\n");
		goto error;
	}

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

	/* Need at least one source and one destination. */
	if (i + 1 >= argc) {
		print_syntax();
		goto error;
	}

	/* All arguments except the last one are sources. */
	do {
		rc = fmgt_flist_append(flist, argv[i++]);
		if (rc != EOK) {
			printf("Out of memory.\n");
			goto error;
		}
	} while (i + 1 < argc);

	dest = argv[i];

	rc = fmgt_create(&fmgt);
	if (rc != EOK) {
		printf("Out of memory.\n");
		goto error;
	}

	fmgt_set_cb(fmgt, &copy_fmgt_cb, NULL);

	rc = fmgt_copy(fmgt, flist, dest);
	if (prog_upd)
		putchar('\n');
	if (rc != EOK) {
		printf("Error copying file(s): %s.\n", str_error(rc));
		goto error;
	}

	fmgt_flist_destroy(flist);
	fmgt_destroy(fmgt);
	return 0;
error:
	if (flist != NULL)
		fmgt_flist_destroy(flist);
	if (fmgt != NULL)
		fmgt_destroy(fmgt);
	return 1;
}

/** @}
 */
