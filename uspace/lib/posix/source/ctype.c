/*
 * Copyright (c) 2011 Jiri Zarevucky
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup libposix
 * @{
 */
/** @file Character classification.
 */

#define LIBPOSIX_INTERNAL
#define __POSIX_DEF__(x) posix_##x

#include "posix/ctype.h"

/**
 * Checks whether character is a hexadecimal digit.
 *
 * @param c Character to inspect.
 * @return Non-zero if character match the definition, zero otherwise.
 */
int posix_isxdigit(int c)
{
	return isdigit(c) ||
	    (c >= 'a' && c <= 'f') ||
	    (c >= 'A' && c <= 'F');
}

/**
 * Checks whether character is a word separator.
 *
 * @param c Character to inspect.
 * @return Non-zero if character match the definition, zero otherwise.
 */
int posix_isblank(int c)
{
	return c == ' ' || c == '\t';
}

/**
 * Checks whether character is a control character.
 *
 * @param c Character to inspect.
 * @return Non-zero if character match the definition, zero otherwise.
 */
int posix_iscntrl(int c)
{
	return c < 0x20 || c == 0x7E;
}

/**
 * Checks whether character is any printing character except space.
 *
 * @param c Character to inspect.
 * @return Non-zero if character match the definition, zero otherwise.
 */
int posix_isgraph(int c)
{
	return posix_isprint(c) && c != ' ';
}

/**
 * Checks whether character is a printing character.
 *
 * @param c Character to inspect.
 * @return Non-zero if character match the definition, zero otherwise.
 */
int posix_isprint(int c)
{
	return posix_isascii(c) && !posix_iscntrl(c);
}

/**
 * Checks whether character is a punctuation.
 *
 * @param c Character to inspect.
 * @return Non-zero if character match the definition, zero otherwise.
 */
int posix_ispunct(int c)
{
	return !isspace(c) && !isalnum(c) && posix_isprint(c);
}

/**
 * Checks whether character is ASCII. (obsolete)
 *
 * @param c Character to inspect.
 * @return Non-zero if character match the definition, zero otherwise.
 */
int posix_isascii(int c)
{
	return c >= 0 && c < 128;
}

/**
 * Converts argument to a 7-bit ASCII character. (obsolete)
 *
 * @param c Character to convert.
 * @return Coverted character.
 */
int posix_toascii(int c)
{
	return c & 0x7F;
}

/** @}
 */
