/*
 * Copyright (c) 2008 Tim Post
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
