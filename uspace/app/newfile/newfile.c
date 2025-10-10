/*
 * Copyright (c) 2025 Jiri Svoboda
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

/** @addtogroup newfile
 * @{
 */
/** @file Create new file.
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

#define NAME  "newfile"

static bool newfile_abort_query(void *);
static void newfile_progress(void *, fmgt_progress_t *);

static bool prog_upd = false;
static bool quiet = false;

static console_ctrl_t *con;

static fmgt_cb_t newfile_fmgt_cb = {
	.abort_query = newfile_abort_query,
	.progress = newfile_progress
};

static void print_syntax(void)
{
	printf("Create new file.\n");
	printf("Syntax: %s [<options] [<file-name>]\n", NAME);
	printf("\t-h           help\n");
	printf("\t-n           non-interactive\n");
	printf("\t-p           create sparse file\n");
	printf("\t-q           quiet\n");
	printf("\t-size=<cap>  file size (<number>[<kB>|<MB>|...])\n");
}

/** Called by fmgt to query for user abort.
 *
 * @param arg Argument (not used)
 * @return @c true iff user requested abort
 */
static bool newfile_abort_query(void *arg)
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
static void newfile_progress(void *arg, fmgt_progress_t *progress)
{
	(void)arg;

	if (!quiet) {
		printf("\rWritten %s of %s (%s done).", progress->curf_procb,
		    progress->curf_totalb, progress->curf_percent);
		fflush(stdout);
		prog_upd = true;
	}
}

int main(int argc, char *argv[])
{
	fmgt_t *fmgt = NULL;
	errno_t rc;
	int i;
	bool nonint = false;
	bool sparse = false;
	char *fsize = NULL;
	char *fname = NULL;
	capa_spec_t fcap;
	uint64_t nbytes = 0;

	con = console_init(stdin, stdout);

	i = 1;
	while (i < argc && argv[i][0] == '-') {
		if (str_cmp(argv[i], "-h") == 0) {
			print_syntax();
			return 0;
		} else if (str_cmp(argv[i], "-n") == 0) {
			++i;
			nonint = true;
		} else if (str_cmp(argv[i], "-p") == 0) {
			++i;
			sparse = true;
		} else if (str_cmp(argv[i], "-q") == 0) {
			++i;
			quiet = true;
		} else if (str_lcmp(argv[i], "-size=",
		    str_length("-size=")) == 0) {
			fsize = argv[i] + str_length("-size=");
			++i;
		} else {
			printf("Invalid option '%s'.\n", argv[i]);
			print_syntax();
			goto error;
		}
	}

	if (i < argc) {
		fname = str_dup(argv[i++]);
		if (fname == NULL) {
			printf("Out of memory.\n");
			goto error;
		}
	}

	if (i < argc) {
		printf("Unexpected argument.\n");
		print_syntax();
		goto error;
	}

	if (fname == NULL) {
		rc = fmgt_new_file_suggest(&fname);
		if (rc != EOK) {
			printf("Out of memory.\n");
			goto error;
		}
	}

	(void)nonint;
	(void)quiet;
	(void)sparse;

	if (fsize != NULL) {
		rc = capa_parse(fsize, &fcap);
		if (rc != EOK) {
			printf("Invalid file size '%s'.\n", fsize);
			goto error;
		}

		rc = capa_to_blocks(&fcap, cv_nom, 1, &nbytes);
		if (rc != EOK) {
			printf("File size too large '%s'.\n", fsize);
			goto error;
		}
	}

	rc = fmgt_create(&fmgt);
	if (rc != EOK) {
		printf("Out of memory.\n");
		goto error;
	}

	fmgt_set_cb(fmgt, &newfile_fmgt_cb, NULL);

	rc = fmgt_new_file(fmgt, fname, nbytes, sparse ? nf_sparse : nf_none);
	if (prog_upd)
		printf("\n");
	if (rc != EOK) {
		printf("Error creating file: %s.\n", str_error(rc));
		goto error;
	}

	free(fname);
	fmgt_destroy(fmgt);
	return 0;
error:
	if (fname != NULL)
		free(fname);
	if (fmgt != NULL)
		fmgt_destroy(fmgt);
	return 1;
}

/** @}
 */
