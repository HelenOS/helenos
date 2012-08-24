/*
 * Copyright (c) 2008 Martin Decky
 * Copyright (c) 2012 Petr Koupy
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

/** @addtogroup genarch
 * @{
 */
/** @file
 */

#include "cursor-11x18.h"

uint8_t cursor_texture[] = {
	0x01, 0x00, 0x03, 0x00, 0x05, 0x00, 0x09, 0x00, 0x11, 0x00, 0x21, 0x00,
	0x41, 0x00, 0x81, 0x00, 0x01, 0x01, 0x01, 0x02, 0x01, 0x04, 0x01, 0x03,
	0x81, 0x00, 0x89, 0x00, 0x15, 0x01, 0x23, 0x01, 0x21, 0x01, 0xc0, 0x00
};

uint8_t cursor_mask[] = {
	0x01, 0x00, 0x03, 0x00, 0x07, 0x00, 0x0f, 0x00, 0x1f, 0x00, 0x3f, 0x00,
	0x7f, 0x00, 0xff, 0x00, 0xff, 0x01, 0xff, 0x03, 0xff, 0x07, 0xff, 0x03,
	0xff, 0x00, 0xff, 0x00, 0xf7, 0x01, 0xe3, 0x01, 0xe1, 0x01, 0xc0, 0x00
};

/** @}
 */
