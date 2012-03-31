/*
 * Copyright (c) 2010 Martin Decky
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

#include <errno.h>
#include <str_error.h>
#include <stdio.h>
#include <fibril.h>

#define MIN_ERRNO  -17
#define NOERR_LEN  64

static const char* err_desc[] = {
	"No error",
	"No such entry",
	"Not enough memory",
	"Limit exceeded", 
	"Connection refused",
	"Forwarding error",
	"Permission denied",
	"Answerbox closed connection",
	"Other party error",
	"Entry already exists",
	"Bad memory pointer",
	"Not supported",
	"Address not available",
	"Timeout expired",
	"Invalid value",
	"Resource is busy",
	"Result does not fit its size",
	"Operation interrupted"
};

static fibril_local char noerr[NOERR_LEN];

const char *str_error(const int e)
{
	if ((e <= 0) && (e >= MIN_ERRNO))
		return err_desc[-e];
	
	/* Ad hoc descriptions of error codes interesting for USB. */
	// FIXME: integrate these as first-class error values
	switch (e) {
		case EBADCHECKSUM:
			return "Bad checksum";
		case ESTALL:
			return "Operation stalled";
		case EAGAIN:
			return "Resource temporarily unavailable";
		case EEMPTY:
			return "Resource is empty";
		default:
			break;
	}

	snprintf(noerr, NOERR_LEN, "Unkown error code %d", e);
	return noerr;
}

/** @}
 */
