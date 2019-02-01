/*
 * Copyright (c) 2006 Martin Decky
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

/** @addtogroup abi_genarch
 * @{
 */
/** @file
 */

#ifndef _ABI_VISUALS_H_
#define _ABI_VISUALS_H_

typedef enum {
	VISUAL_UNKNOWN = 0,
	VISUAL_INDIRECT_8,
	VISUAL_RGB_5_5_5_LE,
	VISUAL_RGB_5_5_5_BE,
	VISUAL_RGB_5_6_5_LE,
	VISUAL_RGB_5_6_5_BE,
	VISUAL_BGR_8_8_8,
	VISUAL_BGR_0_8_8_8,
	VISUAL_BGR_8_8_8_0,
	VISUAL_ABGR_8_8_8_8,
	VISUAL_BGRA_8_8_8_8,
	VISUAL_RGB_8_8_8,
	VISUAL_RGB_0_8_8_8,
	VISUAL_RGB_8_8_8_0,
	VISUAL_ARGB_8_8_8_8,
	VISUAL_RGBA_8_8_8_8
} visual_t;

#endif

/** @}
 */
