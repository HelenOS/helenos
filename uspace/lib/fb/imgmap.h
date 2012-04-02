/*
 * Copyright (c) 2011 Martin Decky
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

/** @addtogroup imgmap
 * @{
 */
/**
 * @file
 */

#ifndef IMGMAP_IMGMAP_H_
#define IMGMAP_IMGMAP_H_

#include <sys/types.h>
#include <abi/fb/visuals.h>
#include <bool.h>
#include "fb.h"

typedef enum {
	IMGMAP_FLAG_NONE = 0,
	IMGMAP_FLAG_SHARED = 1
} imgmap_flags_t;

/** Function to render a pixel. */
typedef void (*pixel2visual_t)(void *, pixel_t);

/** Function to render a bit mask. */
typedef void (*visual_mask_t)(void *, bool);

/** Function to retrieve a pixel. */
typedef pixel_t (*visual2pixel_t)(void *);

extern void pixel2rgb_0888(void *, pixel_t);
extern void pixel2bgr_0888(void *, pixel_t);
extern void pixel2rgb_8880(void *, pixel_t);
extern void pixel2bgr_8880(void *, pixel_t);
extern void pixel2rgb_888(void *, pixel_t);
extern void pixel2bgr_888(void *, pixel_t);
extern void pixel2rgb_555_be(void *, pixel_t);
extern void pixel2rgb_555_le(void *, pixel_t);
extern void pixel2rgb_565_be(void *, pixel_t);
extern void pixel2rgb_565_le(void *, pixel_t);
extern void pixel2bgr_323(void *, pixel_t);
extern void pixel2gray_8(void *, pixel_t);

extern void visual_mask_0888(void *, bool);
extern void visual_mask_8880(void *, bool);
extern void visual_mask_888(void *, bool);
extern void visual_mask_555(void *, bool);
extern void visual_mask_565(void *, bool);
extern void visual_mask_323(void *, bool);
extern void visual_mask_8(void *, bool);

extern pixel_t rgb_0888_2pixel(void *);
extern pixel_t bgr_0888_2pixel(void *);
extern pixel_t rgb_8880_2pixel(void *);
extern pixel_t bgr_8880_2pixel(void *);
extern pixel_t rgb_888_2pixel(void *);
extern pixel_t bgr_888_2pixel(void *);
extern pixel_t rgb_555_be_2pixel(void *);
extern pixel_t rgb_555_le_2pixel(void *);
extern pixel_t rgb_565_be_2pixel(void *);
extern pixel_t rgb_565_le_2pixel(void *);
extern pixel_t bgr_323_2pixel(void *);
extern pixel_t gray_8_2pixel(void *);

extern imgmap_t *imgmap_create(sysarg_t, sysarg_t, visual_t, imgmap_flags_t);
extern void imgmap_put_pixel(imgmap_t *, sysarg_t, sysarg_t, pixel_t);
extern pixel_t imgmap_get_pixel(imgmap_t *, sysarg_t, sysarg_t);

extern imgmap_t *imgmap_decode_tga(void *, size_t, imgmap_flags_t);
extern void imgmap_get_resolution(imgmap_t *, sysarg_t *, sysarg_t *);

#endif

/** @}
 */
