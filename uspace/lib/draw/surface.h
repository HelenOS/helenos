/*
 * Copyright (c) 2011 Martin Decky
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

/** @addtogroup draw
 * @{
 */
/**
 * @file
 */

#ifndef DRAW_SURFACE_H_
#define DRAW_SURFACE_H_

#include <stdbool.h>
#include <io/pixelmap.h>

struct surface;
typedef struct surface surface_t;

typedef sysarg_t surface_coord_t;

typedef enum {
	SURFACE_FLAG_NONE = 0,
	SURFACE_FLAG_SHARED = 1
} surface_flags_t;

extern surface_t *surface_create(surface_coord_t, surface_coord_t, pixel_t *, surface_flags_t);
extern void surface_destroy(surface_t *);

extern bool surface_is_shared(surface_t *);
extern pixel_t *surface_direct_access(surface_t *);
extern pixelmap_t *surface_pixmap_access(surface_t *);
extern void surface_get_resolution(surface_t *, surface_coord_t *, surface_coord_t *);
extern void surface_get_damaged_region(surface_t *, surface_coord_t *, surface_coord_t *,
    surface_coord_t *, surface_coord_t *);
extern void surface_add_damaged_region(surface_t *, surface_coord_t, surface_coord_t,
    surface_coord_t, surface_coord_t);
extern void surface_reset_damaged_region(surface_t *);

extern void surface_put_pixel(surface_t *, surface_coord_t, surface_coord_t, pixel_t);
extern pixel_t surface_get_pixel(surface_t *, surface_coord_t, surface_coord_t);

#endif

/** @}
 */
