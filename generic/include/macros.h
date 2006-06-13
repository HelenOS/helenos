/*
 * Copyright (C) 2005 Jakub Jermar
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

 /** @addtogroup generic	
 * @{
 */
/** @file
 */

#ifndef __MACROS_H__
#define __MACROS_H__

#define is_digit(d)	(((d) >= '0') && ((d)<='9'))
#define is_lower(c)	(((c) >= 'a') && ((c) <= 'z'))
#define is_upper(c)	(((c) >= 'A') && ((c) <= 'Z'))
#define is_alpha(c)	(is_lower(c) || is_upper(c))
#define is_alphanum(c)	(is_alpha(c) || is_digit(c))
#define is_white(c)	(((c) == ' ') || ((c) == '\t') || ((c) == '\n') || ((c) == '\r'))

#define min(a,b)	((a)<(b)?(a):(b))
#define max(a,b)	((a)>(b)?(a):(b))

/** Return true if the interlvals overlap. */
static inline int overlaps(__address s1, size_t sz1, __address s2, size_t sz2)
{
	__address e1 = s1+sz1;
	__address e2 = s2+sz2;

	return s1 < e2 && s2 < e1;
}
/* Compute overlapping of physical addresses */
#define PA_overlaps(x,szx,y,szy)  overlaps(KA2PA(x),szx,KA2PA(y), szy)

#endif

 /** @}
 */

