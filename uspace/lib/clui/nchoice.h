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
