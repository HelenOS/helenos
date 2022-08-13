/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/** @addtogroup libclui
 * @{
 */
/**
 * @file Numerical choice
 */

#include <errno.h>
#include <nchoice.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <tinput.h>

/** Create numerical choice
 *
 */
errno_t nchoice_create(nchoice_t **rchoice)
{
	nchoice_t *choice = NULL;
	errno_t rc;

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
errno_t nchoice_set_prompt(nchoice_t *choice, const char *prompt)
{
	char *pstr;

	pstr = str_dup(prompt);
	if (pstr == NULL)
		return ENOMEM;

	free(choice->prompt);
	choice->prompt = pstr;
	return EOK;
}

/** Add option to numerical choice */
errno_t nchoice_add(nchoice_t *choice, const char *opttext, void *arg,
    nchoice_flag_t flags)
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

	if ((flags & ncf_default) != 0) {
		/* Set this option as the default */
		assert(choice->def_opt == NULL);

		choice->def_opt = opt;
	}

	return EOK;
}

/** Get numerical choice from user */
errno_t nchoice_get(nchoice_t *choice, void **rarg)
{
	int i;
	errno_t rc;
	int ret;
	char *str;
	unsigned long c;
	char *eptr;
	char *istr;
	int def_i;

again:
	printf("%s\n", choice->prompt);

	def_i = 0;
	i = 1;
	list_foreach(choice->opts, lchoice, nchoice_opt_t, opt) {
		printf("%d: %s%s\n", i, opt->text, opt == choice->def_opt ?
		    " [default]" : "");
		if (opt == choice->def_opt)
			def_i = i;
		++i;
	}

	if (def_i > 0) {
		ret = asprintf(&istr, "%d", def_i);
		if (ret < 0)
			return ENOMEM;
	} else {
		istr = str_dup("");
		if (istr == NULL)
			return ENOMEM;
	}

	rc = tinput_read_i(choice->tinput, istr, &str);
	free(istr);

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
