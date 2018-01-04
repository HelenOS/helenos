/*
 * Copyright (c) 2010 Martin Decky
 * Copyright (c) 2017 CZ.NIC, z.s.p.o.
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

#include <errno.h>
#include <str.h>

/* The arrays below are automatically generated from the same file that
 * errno.h constants are generated from. Triple-include of the same list
 * with redefinitions of __errno() macro are used to ensure that the
 * information cannot get out of synch. This is inpired by musl libc.
 */

#undef __errno_entry
#define __errno_entry(name, num, desc) name,

static const errno_t err_num[] = {
#include <abi/errno.in>
};

#undef __errno_entry
#define __errno_entry(name, num, desc) #name,

static const char* err_name[] = {
#include <abi/errno.in>
};

#undef __errno_entry
#define __errno_entry(name, num, desc) "[" #name "]" desc,

static const char* err_desc[] = {
#include <abi/errno.in>
};

/* Returns index corresponding to the given errno, or -1 if not found. */
static int find_errno(errno_t e)
{
	/* Just a dumb linear search.
	 * There too few entries to warrant anything smarter.
	 */

	int len = sizeof(err_num) / sizeof(errno_t);

	for (int i = 0; i < len; i++) {
		if (err_num[i] == e) {
			return i;
		}
	}

	return -1;
}

const char *str_error_name(errno_t e)
{
	int i = find_errno(e);

	if (i >= 0) {
		return err_name[i];
	}

	return "(unknown)";
}

const char *str_error(errno_t e)
{
	int i = find_errno(e);

	if (i >= 0) {
		return err_desc[i];
	}

	return "Unknown error code";
}

