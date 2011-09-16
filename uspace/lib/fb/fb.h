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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef IMGMAP_FB_H_
#define IMGMAP_FB_H_

#include <sys/types.h>
#include <async.h>
#include <io/style.h>
#include <io/console.h>
#include <ipc/common.h>

typedef enum {
	/** Screen methods */
	FB_GET_RESOLUTION = IPC_FIRST_USER_METHOD,
	FB_YIELD,
	FB_CLAIM,
	FB_POINTER_UPDATE,
	
	/** Object methods */
	FB_VP_CREATE,
	FB_VP_DESTROY,
	FB_FRONTBUF_CREATE,
	FB_FRONTBUF_DESTROY,
	FB_IMAGEMAP_CREATE,
	FB_IMAGEMAP_DESTROY,
	FB_SEQUENCE_CREATE,
	FB_SEQUENCE_DESTROY,
	FB_SEQUENCE_ADD_IMAGEMAP,
	
	/** Viewport stateful methods */
	FB_VP_FOCUS,
	FB_VP_CLEAR,
	FB_VP_GET_DIMENSIONS,
	FB_VP_GET_CAPS,
	
	/** Style methods (viewport specific) */
	FB_VP_CURSOR_UPDATE,
	FB_VP_SET_STYLE,
	FB_VP_SET_COLOR,
	FB_VP_SET_RGB_COLOR,
	
	/** Text methods (viewport specific) */
	FB_VP_PUTCHAR,
	FB_VP_UPDATE,
	FB_VP_DAMAGE,
	
	/** Imagemap methods (viewport specific) */
	FB_VP_IMAGEMAP_DAMAGE,
	
	/** Sequence methods (viewport specific) */
	FB_VP_SEQUENCE_START,
	FB_VP_SEQUENCE_STOP
} fb_request_t;

/** Forward declarations */
struct screenbuffer;
struct imgmap;

typedef struct screenbuffer screenbuffer_t;
typedef struct imgmap imgmap_t;

typedef uint32_t pixel_t;

typedef sysarg_t vp_handle_t;
typedef sysarg_t frontbuf_handle_t;
typedef sysarg_t imagemap_handle_t;
typedef sysarg_t sequence_handle_t;

extern int fb_get_resolution(async_sess_t *, sysarg_t *, sysarg_t *);
extern int fb_yield(async_sess_t *);
extern int fb_claim(async_sess_t *);
extern int fb_pointer_update(async_sess_t *, sysarg_t, sysarg_t, bool);

extern vp_handle_t fb_vp_create(async_sess_t *, sysarg_t, sysarg_t, sysarg_t,
    sysarg_t);
extern frontbuf_handle_t fb_frontbuf_create(async_sess_t *, screenbuffer_t *);
extern imagemap_handle_t fb_imagemap_create(async_sess_t *, imgmap_t *);
extern sequence_handle_t fb_sequence_create(async_sess_t *);

extern int fb_vp_get_dimensions(async_sess_t *, vp_handle_t, sysarg_t *,
    sysarg_t *);
extern int fb_vp_get_caps(async_sess_t *, vp_handle_t, console_caps_t *);
extern int fb_vp_clear(async_sess_t *, vp_handle_t);

extern int fb_vp_cursor_update(async_sess_t *, vp_handle_t, frontbuf_handle_t);
extern int fb_vp_set_style(async_sess_t *, vp_handle_t, console_style_t);

extern int fb_vp_putchar(async_sess_t *, vp_handle_t, sysarg_t, sysarg_t,
    wchar_t);
extern int fb_vp_update(async_sess_t *, vp_handle_t, frontbuf_handle_t);
extern int fb_vp_damage(async_sess_t *, vp_handle_t, frontbuf_handle_t,
    sysarg_t, sysarg_t, sysarg_t, sysarg_t);

extern int fb_vp_imagemap_damage(async_sess_t *, vp_handle_t, imagemap_handle_t,
    sysarg_t, sysarg_t, sysarg_t, sysarg_t);

extern int fb_sequence_add_imagemap(async_sess_t *, sequence_handle_t,
    imagemap_handle_t);
extern int fb_vp_sequence_start(async_sess_t *, vp_handle_t, sequence_handle_t);

#endif

/** @}
 */
