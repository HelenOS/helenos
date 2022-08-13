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

#ifndef LIBCLUI_NCHOICE_H_
#define LIBCLUI_NCHOICE_H_

#include <adt/list.h>
#include <tinput.h>

typedef enum {
	/** This is the default option */
	ncf_default = 1
} nchoice_flag_t;

typedef struct {
	/** Link to nchoice_t.opts */
	link_t lchoice;
	/** Option text */
	char *text;
	/** User argument */
	void *arg;
} nchoice_opt_t;

typedef struct {
	/** Prompt text */
	char *prompt;
	/** Options */
	list_t opts; /* of nchoice_opt_t */
	/** Text input */
	tinput_t *tinput;
	/** Default option */
	nchoice_opt_t *def_opt;
} nchoice_t;

extern errno_t nchoice_create(nchoice_t **);
extern void nchoice_destroy(nchoice_t *);
extern errno_t nchoice_set_prompt(nchoice_t *, const char *);
extern errno_t nchoice_add(nchoice_t *, const char *, void *, nchoice_flag_t);
extern errno_t nchoice_get(nchoice_t *, void **);

#endif

/** @}
 */
