/*
 * Copyright (c) 2006 Josef Cejka
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

#ifndef LIBC_CTYPE_H_
#define LIBC_CTYPE_H_

static inline int islower(int c)
{
	return ((c >= 'a') && (c <= 'z'));
}

static inline int isupper(int c)
{
	return ((c >= 'A') && (c <= 'Z'));
}

static inline int isalpha(int c)
{
	return (islower(c) || isupper(c));
}

static inline int isdigit(int c)
{
	return ((c >= '0') && (c <= '9'));
}

static inline int isalnum(int c)
{
	return (isalpha(c) || isdigit(c));
}

static inline int isspace(int c)
{
	switch (c) {
	case ' ':
	case '\n':
	case '\t':
	case '\f':
	case '\r':
	case '\v':
		return 1;
		break;
	default:
		return 0;
	}
}

static inline int tolower(int c)
{
	if (isupper(c))
		return (c + ('a' - 'A'));
	else
		return c;
}

static inline int toupper(int c)
{
	if (islower(c))
		return (c + ('A' - 'a'));
	else
		return c;
}

#endif

/** @}
 */
