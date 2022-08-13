/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup softrend
 * @{
 */
/**
 * @file
 */

#ifndef SOFTREND_PIXCONV_H_
#define SOFTREND_PIXCONV_H_

#include <stdbool.h>
#include <io/pixel.h>

/** Function to render a pixel. */
typedef void (*pixel2visual_t)(void *, pixel_t);

/** Function to render a bit mask. */
typedef void (*visual_mask_t)(void *, bool);

/** Function to retrieve a pixel. */
typedef pixel_t (*visual2pixel_t)(void *);

extern void pixel2argb_8888(void *, pixel_t);
extern void pixel2abgr_8888(void *, pixel_t);
extern void pixel2rgba_8888(void *, pixel_t);
extern void pixel2bgra_8888(void *, pixel_t);
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

extern void visual_mask_8888(void *, bool);
extern void visual_mask_0888(void *, bool);
extern void visual_mask_8880(void *, bool);
extern void visual_mask_888(void *, bool);
extern void visual_mask_555(void *, bool);
extern void visual_mask_565(void *, bool);
extern void visual_mask_323(void *, bool);
extern void visual_mask_8(void *, bool);

extern pixel_t argb_8888_2pixel(void *);
extern pixel_t abgr_8888_2pixel(void *);
extern pixel_t rgba_8888_2pixel(void *);
extern pixel_t bgra_8888_2pixel(void *);
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

#endif

/** @}
 */
