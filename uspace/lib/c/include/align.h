/*
 * Copyright (c) 2005 Jakub Jermar
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

#ifndef _LIBC_ALIGN_H_
#define _LIBC_ALIGN_H_

/** Align to the nearest lower address which is a power of two.
 *
 * @param s		Address or size to be aligned.
 * @param a		Size of alignment, must be power of 2.
 */
#define ALIGN_DOWN(s, a)	((s) & ~((typeof(s))(a) - 1))

/** Align to the nearest higher address which is a power of two.
 *
 * @param s		Address or size to be aligned.
 * @param a		Size of alignment, must be power of 2.
 */
#define ALIGN_UP(s, a)		((((s) + ((a) - 1)) & ~((typeof(s))(a) - 1)))

/** Round up to the nearest higher boundary.
 *
 * @param n		Number to be aligned.
 * @param b		Boundary, arbitrary unsigned number.
 */
#define ROUND_UP(n, b)		(((n) / (b) + ((n) % (b) != 0)) * (b))

#endif

/** @}
 */
