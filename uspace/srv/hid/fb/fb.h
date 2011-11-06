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

#ifndef FB_FB_H_
#define FB_FB_H_

#include <sys/types.h>
#include <bool.h>
#include <loc.h>
#include <io/console.h>
#include <io/style.h>
#include <io/color.h>
#include <fb.h>
#include <screenbuffer.h>
#include <imgmap.h>

struct fbdev;
struct fbvp;

typedef struct {
	int (* yield)(struct fbdev *dev);
	int (* claim)(struct fbdev *dev);
	void (* pointer_update)(struct fbdev *dev, sysarg_t x, sysarg_t y,
	    bool visible);
	
	int (* get_resolution)(struct fbdev *dev, sysarg_t *width,
	    sysarg_t *height);
	void (* font_metrics)(struct fbdev *dev, sysarg_t width,
	    sysarg_t height, sysarg_t *cols, sysarg_t *rows);
	
	int (* vp_create)(struct fbdev *dev, struct fbvp *vp);
	void (* vp_destroy)(struct fbdev *dev, struct fbvp *vp);
	
	void (* vp_clear)(struct fbdev *dev, struct fbvp *vp);
	console_caps_t (* vp_get_caps)(struct fbdev *dev, struct fbvp *vp);
	
	void (* vp_cursor_update)(struct fbdev *dev, struct fbvp *vp,
	    sysarg_t prev_col, sysarg_t prev_row, sysarg_t col, sysarg_t row,
	    bool visible);
	void (* vp_cursor_flash)(struct fbdev *dev, struct fbvp *vp,
	    sysarg_t col, sysarg_t row);
	
	void (* vp_char_update)(struct fbdev *dev, struct fbvp *vp, sysarg_t col,
	    sysarg_t row);
	
	void (* vp_imgmap_damage)(struct fbdev *dev, struct fbvp *vp,
	    imgmap_t *imgmap, sysarg_t col, sysarg_t row, sysarg_t cols,
	    sysarg_t rows);
} fbdev_ops_t;

typedef struct fbdev {
	link_t link;
	
	atomic_t refcnt;
	bool claimed;
	
	sysarg_t index;
	service_id_t dsid;
	struct fbvp *active_vp;
	
	list_t vps;
	list_t frontbufs;
	list_t imagemaps;
	list_t sequences;
	
	fbdev_ops_t ops;
	void *data;
} fbdev_t;

typedef struct fbvp {
	link_t link;
	
	sysarg_t x;
	sysarg_t y;
	
	sysarg_t width;
	sysarg_t height;
	
	sysarg_t cols;
	sysarg_t rows;
	
	char_attrs_t attrs;
	list_t sequences;
	
	screenbuffer_t *backbuf;
	sysarg_t top_row;
	
	bool cursor_active;
	bool cursor_flash;
	
	void *data;
} fbvp_t;

typedef struct {
	link_t link;
	
	size_t size;
	unsigned int flags;
	void *data;
} frontbuf_t;

typedef struct {
	link_t link;
	link_t seq_link;
	
	size_t size;
	unsigned int flags;
	void *data;
} imagemap_t;

typedef struct {
	link_t link;
	
	list_t imagemaps;
	size_t count;
} sequence_t;

typedef struct {
	link_t link;
	
	sequence_t *seq;
	size_t current;
} sequence_vp_t;

extern fbdev_t *fbdev_register(fbdev_ops_t *, void *);

#endif
