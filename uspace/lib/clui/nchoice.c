/*
 * Copyright (c) 2015 Jiri Svoboda
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
/** @addtogroup libclui
 * @{
 */
/**
 * @file Numerical choice
 */

#include <errno.h>
#include <nchoice.h>
#include <stdlib.h>
#include <str.h>
#include <tinput.h>

/** Create numerical choice
 *
 */
int nchoice_create(nchoice_t **rchoice)
{
	nchoice_t *choice = NULL;
	int rc;

	choice = calloc(1, sizeof(nchoice_t));
	if (choice == NULL)
		goto error;

	list_initialize(&choice->opts);

	choice->tinput = tinput_new();
	if (choice->tinput == NULL)
		goto error;

	rc = tinput_set_prompt(choice->tinput, "Choice> ");
	if (rc != EOK) {
		assert(rc == ENOMEM);
		goto error;
	}

	*rchoice = choice;
	return EOK;
error:
	nchoice_destroy(choice);
	return ENOMEM;
}

/** Destroy numerical choice */
void nchoice_destroy(nchoice_t *choice)
{
	if (choice == NULL)
		return;

	tinput_destroy(choice->tinput);
	free(choice);
}

/** Set numerica choice prompt text */
int nchoice_set_prompt(nchoice_t *choice, const char *prompt)
{
	char *pstr;

	pstr = str_dup(prompt);
	if (pstr == NULL)
		return ENOMEM;

	free(choice->prompt);
	choice->prompt = pstr;
	return EOK;
}

#include <stdio.h>
/** Add option to numerical choice */
int nchoice_add(nchoice_t *choice, const char *opttext, void *arg)
{
	nchoice_opt_t *opt;
	char *ptext;

	opt = calloc(1, sizeof(nchoice_opt_t));
	if (opt == NULL)
		return ENOMEM;

	ptext = str_dup(opttext);
	if (ptext == NULL) {
		free(opt);
		return ENOMEM;
	}

	opt->text = ptext;
	opt->arg = arg;
	list_append(&opt->lchoice, &choice->opts);
	return EOK;
}

/** Get numerical choice from user */
int nchoice_get(nchoice_t *choice, void **rarg)
{
	int i;
	int rc;
	char *str;
	unsigned long c;
	char *eptr;

again:
	printf("%s\n", choice->prompt);

	i = 1;
	list_foreach(choice->opts, lchoice, nchoice_opt_t, opt) {
		printf("%d: %s\n", i, opt->text);
		++i;
	}

	rc = tinput_read(choice->tinput, &str);
	if (rc != EOK) {
		assert(rc == EIO || rc == ENOENT);

		if (rc == ENOENT) {
			return ENOENT;
		} else {
			/* rc == EIO */
			return EIO;
		}
	}

	c = strtoul(str, &eptr, 10);
	if (*eptr != '\0' || c < 1 || c > (unsigned long)i - 1) {
		printf("Invalid choice. Try again.\n");
		free(str);
		goto again;
	}

	free(str);

	i = 1;
	list_foreach(choice->opts, lchoice, nchoice_opt_t, opt) {
		if ((unsigned long)i == c) {
			*rarg = opt->arg;
			return EOK;
		}
		++i;
	}

	assert(false);
	return ENOENT;
}

/** @}
 */
