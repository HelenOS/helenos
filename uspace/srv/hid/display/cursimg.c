/*
 * Copyright (c) 2021 Jiri Svoboda
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

/** @addtogroup display
 * @{
 */
/**
 * @file Display server built-in cursor images
 */

#include <types/display/cursor.h>
#include "cursimg.h"

static uint8_t arrow_img[] = {
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0,
	1, 2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 1,
	1, 2, 2, 2, 1, 2, 2, 1, 0, 0, 0, 0, 0,
	1, 2, 2, 1, 0, 1, 2, 2, 1, 0, 0, 0, 0,
	1, 2, 1, 0, 0, 1, 2, 2, 1, 0, 0, 0, 0,
	1, 1, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0, 0,
	1, 0, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0
};

static uint8_t size_ud_img[] = {
	0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
	0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0,
	0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0,
	1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1,
	0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
	0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
	0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
	0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
	0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
	0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
	0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
	1, 1, 1, 1, 2, 2, 2, 2, 2, 1, 1, 1, 1,
	0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0,
	0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0,
	0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 2, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0
};

static uint8_t size_lr_img[] = {
	0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 0, 0, 0,
	0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0,
	0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0,
	1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1,
	0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0,
	0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0,
	0, 0, 0, 1, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0
};

static uint8_t size_uldr_img[] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 1,
	1, 2, 2, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 1, 1,
	1, 2, 1, 0, 1, 2, 2, 2, 2, 2, 1, 0, 1, 2, 1,
	1, 1, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 2, 2, 1,
	1, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 1,
	0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1,
	0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 1,
	0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 1,
	0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

static uint8_t size_urdl_img[] = {
	0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 1,
	0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1,
	0, 0, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 1,
	1, 0, 0, 0, 0, 0, 1, 2, 2, 2, 2, 2, 2, 2, 1,
	1, 1, 0, 0, 0, 1, 2, 2, 2, 2, 2, 1, 2, 2, 1,
	1, 2, 1, 0, 1, 2, 2, 2, 2, 2, 1, 0, 1, 2, 1,
	1, 2, 2, 1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 1, 1,
	1, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 1,
	1, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0,
	1, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0
};

static uint8_t ibeam_img[] = {
	0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0,
	1, 2, 2, 2, 2, 1, 2, 2, 2, 2, 1,
	0, 1, 1, 1, 1, 2, 1, 1, 1, 1, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 2, 1, 0, 0, 0, 0,
	0, 1, 1, 1, 1, 2, 1, 1, 1, 1, 0,
	1, 2, 2, 2, 2, 1, 2, 2, 2, 2, 1,
	0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0
};

ds_cursimg_t ds_cursimg[dcurs_limit] = {
	{
		.rect = { 0, 0, 13, 21 },
		.image = arrow_img
	},
	{
		.rect = { -6, -10, 7, 11 },
		.image = size_ud_img
	},
	{
		.rect = { -10, -6, 11, 7 },
		.image = size_lr_img
	},
	{
		.rect = { -7, -7, 8, 8 },
		.image = size_uldr_img
	},
	{
		.rect = { -7, -7, 8, 8 },
		.image = size_urdl_img
	},
	{
		.rect = { -5, -9, 6, 10 },
		.image = ibeam_img
	}
};

/** @}
 */
