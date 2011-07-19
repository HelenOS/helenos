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

#ifndef POSIX_CTYPE_H_
#define POSIX_CTYPE_H_

#include "libc/ctype.h"

/* Classification of Characters */
extern int posix_isxdigit(int c);
extern int posix_isblank(int c);
extern int posix_iscntrl(int c);
extern int posix_isgraph(int c);
extern int posix_isprint(int c);
extern int posix_ispunct(int c);

/* Obsolete Functions and Macros */
extern int posix_isascii(int c);
extern int posix_toascii(int c);
#undef _tolower
#define _tolower(c) ((c) - 'A' + 'a')
#undef _toupper
#define _toupper(c) ((c) - 'a' + 'A')


#ifndef LIBPOSIX_INTERNAL
	#define isxdigit posix_isxdigit
	#define isblank posix_isblank
	#define iscntrl posix_iscntrl
	#define isgraph posix_isgraph
	#define isprint posix_isprint
	#define ispunct posix_ispunct
	
	#define isascii posix_isascii
	#define toascii posix_toascii
#endif

#endif /* POSIX_CTYPE_H_ */

/** @}
 */
