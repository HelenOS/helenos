/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 * SPDX-FileCopyrightText: 2014 Martin Decky
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libgfximage
 * @{
 */
/**
 * @file
 */

#ifndef GFXIMAGE_TGA_GZ_H_
#define GFXIMAGE_TGA_GZ_H_

#include <errno.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <stddef.h>

extern errno_t decode_tga_gz(gfx_context_t *, void *, size_t,
    gfx_bitmap_t **, gfx_rect_t *);

#endif

/** @}
 */
