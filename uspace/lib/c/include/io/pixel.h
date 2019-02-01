/*
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

/** @addtogroup libc
 * @{
 */
/**
 * @file
 */

#ifndef _LIBC_IO_PIXEL_H_
#define _LIBC_IO_PIXEL_H_

#include <stdint.h>

#define NARROW(channel, bits) \
	((channel) >> (8 - (bits)))

#define INVERT(pixel) ((pixel) ^ 0x00ffffff)

#define ALPHA(pixel)  ((pixel) >> 24)
#define RED(pixel)    (((pixel) & 0x00ff0000) >> 16)
#define GREEN(pixel)  (((pixel) & 0x0000ff00) >> 8)
#define BLUE(pixel)   ((pixel) & 0x000000ff)

#define PIXEL(a, r, g, b) \
	((((unsigned)(a) & 0xff) << 24) | (((unsigned)(r) & 0xff) << 16) | \
	(((unsigned)(g) & 0xff) << 8) | ((unsigned)(b) & 0xff))

typedef uint32_t pixel_t;

#endif

/** @}
 */
