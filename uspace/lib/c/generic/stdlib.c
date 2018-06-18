/*
 * Copyright (c) 2006 Ondrej Palkovsky
 * Copyright (c) 2018 Jiri Svoboda
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <adt/list.h>
#include <fibril_synch.h>
#include <stdlib.h>
#include "private/libc.h"
#include "private/stdlib.h"

static int glbl_seed = 1;

static LIST_INITIALIZE(exit_handlers);
static FIBRIL_MUTEX_INITIALIZE(exit_handlers_lock);

static LIST_INITIALIZE(quick_exit_handlers);
static FIBRIL_MUTEX_INITIALIZE(quick_exit_handlers_lock);


int rand(void)
{
	return glbl_seed = ((1366 * glbl_seed + 150889) % RAND_MAX);
}

void srand(unsigned int seed)
{
	glbl_seed = seed % RAND_MAX;
}

/** Register exit handler.
 *
 * @param func Function to be called during program terimnation
 * @return Zero on success, nonzero on failure
 */
int atexit(void (*func)(void))
{
	__exit_handler_t *entry;

	entry = malloc(sizeof(__exit_handler_t));
	if (entry == NULL)
		return -1;

	entry->func = func;

	fibril_mutex_lock(&exit_handlers_lock);
	list_prepend(&entry->llist, &exit_handlers);
	fibril_mutex_unlock(&exit_handlers_lock);
	return 0;
}

/** Terminate program with exit status.
 *
 * @param status Exit status
 */
void exit(int status)
{
	link_t *link;
	__exit_handler_t *eh;

	/* Call exit handlers */
	fibril_mutex_lock(&exit_handlers_lock);
	while (!list_empty(&exit_handlers)) {
		link = list_first(&exit_handlers);
		list_remove(link);
		fibril_mutex_unlock(&exit_handlers_lock);

		eh = list_get_instance(link, __exit_handler_t, llist);
		eh->func();
		free(eh);
		fibril_mutex_lock(&exit_handlers_lock);
	}

	fibril_mutex_unlock(&exit_handlers_lock);

	_Exit(status);
}

/** Register quick exit handler.
 *
 * @param func Function to be called during quick program terimnation
 * @return Zero on success, nonzero on failure
 */
int at_quick_exit(void (*func)(void))
{
	__exit_handler_t *entry;

	entry = malloc(sizeof(__exit_handler_t));
	if (entry == NULL)
		return -1;

	entry->func = func;

	fibril_mutex_lock(&exit_handlers_lock);
	list_prepend(&entry->llist, &exit_handlers);
	fibril_mutex_unlock(&exit_handlers_lock);
	return 0;
}

/** Quickly terminate program with exit status.
 *
 * @param status Exit status
 */
void quick_exit(int status)
{
	link_t *link;
	__exit_handler_t *eh;

	/* Call quick exit handlers */
	fibril_mutex_lock(&quick_exit_handlers_lock);
	while (!list_empty(&quick_exit_handlers)) {
		link = list_first(&quick_exit_handlers);
		list_remove(link);
		fibril_mutex_unlock(&quick_exit_handlers_lock);

		eh = list_get_instance(link, __exit_handler_t, llist);
		eh->func();
		free(eh);
		fibril_mutex_lock(&quick_exit_handlers_lock);
	}

	fibril_mutex_unlock(&quick_exit_handlers_lock);

	_Exit(status);
}

void _Exit(int status)
{
	__libc_exit(status);
}

/** Abnormal program termination */
void abort(void)
{
	__libc_abort();
}

/** Get environment list entry.
 *
 * Note that this function is not reentrant. The returned string is only
 * guaranteed to be valid until the next call to @c getenv.
 *
 * @param name Entry name
 * @return Pointer to string or @c NULL if not found
 */
char *getenv(const char *name)
{
	(void) name;
	return NULL;
}

/** Execute command.
 *
 * @param string Command to execute or @c NULL
 *
 * @return If @a string is @c NULL, return zero (no command processor
 *         available). If @a string is not @c NULL, return 1 (failure).
 */
int system(const char *string)
{
	if (string == NULL)
		return 0;

	return 1;
}

/** Compute quotient and remainder of int division.
 *
 * @param numer Numerator
 * @param denom Denominator
 * @return Structure containing quotient and remainder
 */
div_t div(int numer, int denom)
{
	div_t d;

	d.quot = numer / denom;
	d.rem = numer % denom;

	return d;
}

/** Compute quotient and remainder of long division.
 *
 * @param numer Numerator
 * @param denom Denominator
 * @return Structure containing quotient and remainder
 */
ldiv_t ldiv(long numer, long denom)
{
	ldiv_t d;

	d.quot = numer / denom;
	d.rem = numer % denom;

	return d;
}

/** Compute quotient and remainder of long long division.
 *
 * @param numer Numerator
 * @param denom Denominator
 * @return Structure containing quotient and remainder
 */
lldiv_t lldiv(long long numer, long long denom)
{
	lldiv_t d;

	d.quot = numer / denom;
	d.rem = numer % denom;

	return d;
}

/** @}
 */
