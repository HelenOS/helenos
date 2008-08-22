/* Copyright (c) 2008, Tim Post <tinkertim@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of the original program's authors nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

#include "config.h"
#include "errors.h"
#include "errstr.h"

/* Error printing, translation and handling functions */

volatile int cli_lasterr = 0;
extern volatile unsigned int cli_verbocity;

/* Look up errno in cl_errors and return the corresponding string.
 * Return NULL if not found */
char *err2str(int errno)
{

	if (NULL != cl_errors[errno])
		return cl_errors[errno];

	return (char *)NULL;
}

/* Print an error report signifying errno, which is translated to
 * its corresponding human readable string. If errno > 0, raise the
 * cli_quit int that tells the main program loop to exit immediately */

void cli_error(int errno, const char *fmt, ...)
{
	va_list vargs;
	va_start(vargs, fmt);
	vprintf(fmt, vargs);
	va_end(vargs);

	if (NULL != err2str(errno))
		printf(" (%s)\n", err2str(errno));
	else
		printf(" (Unknown Error %d)\n", errno);

	if (errno < 0)
		exit(EXIT_FAILURE);

}

/* Just a smart printf(), print the string only if cli_verbocity is high */
void cli_verbose(const char *fmt, ...)
{
	if (cli_verbocity) {
		va_list vargs;

		printf("[*] ");
		va_start(vargs, fmt);
		vprintf(fmt, vargs);
		va_end(vargs);
		printf("\n");
	}
	return;
}




