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

#ifndef LIBC_IO_MODE_H_
#define LIBC_IO_MODE_H_

#include <abi/fb/visuals.h>
#include <adt/list.h>
#include <io/pixel.h>
#include <io/charfield.h>

typedef struct {
	sysarg_t width;
	sysarg_t height;
} aspect_ratio_t;

typedef union {
	visual_t pixel_visual;
	char_attr_type_t field_visual;
} cell_visual_t;

typedef struct {
	sysarg_t index;
	sysarg_t version;

	sysarg_t refresh_rate;
	aspect_ratio_t screen_aspect;
	sysarg_t screen_width;
	sysarg_t screen_height;

	aspect_ratio_t cell_aspect;
	cell_visual_t cell_visual;
} vslmode_t;

typedef struct {
	link_t link;
	vslmode_t mode;
} vslmode_list_element_t;

#endif

/** @}
 */
