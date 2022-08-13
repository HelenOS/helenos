/*
 * SPDX-FileCopyrightText: 2008 Tim Post
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <str.h>

#include "config.h"
#include "errors.h"
#include "errstr.h"
#include "scli.h"

volatile int cli_errno = CL_EOK;
extern volatile unsigned int cli_quit;

/* Error printing, translation and handling functions */

/** Look up errno in cl_errors and return the corresponding string.
 *
 * Return NULL if not found
 */
static const char *err2str(int err)
{

	if (NULL != cl_errors[err])
		return cl_errors[err];

	return (char *)NULL;
}

/** Print an error report signifying errno
 *
 * errno is translated to its corresponding human readable string.
 * If errno > 0, raise the cli_quit int that tells the main program loop
 * to exit immediately
 */
void cli_error(int err, const char *fmt, ...)
{
	va_list vargs;
	va_start(vargs, fmt);
	vprintf(fmt, vargs);
	va_end(vargs);

	if (NULL != err2str(err))
		printf(" (%s)\n", err2str(err));
	else
		printf(" (Unknown Error %d)\n", err);

	/*
	 * If fatal, raise cli_quit so that we try to exit
	 * gracefully. This will break the main loop and
	 * invoke the destructor
	 */
	if (err == CL_EFATAL)
		cli_quit = 1;

	return;

}
