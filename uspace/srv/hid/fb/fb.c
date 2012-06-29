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

#include <sys/types.h>
#include <bool.h>
#include <loc.h>
#include <errno.h>
#include <stdio.h>
#include <malloc.h>
#include <inttypes.h>
#include <as.h>
#include <fb.h>
#include <screenbuffer.h>
#include "port/ega.h"
#include "port/kchar.h"
#include "port/kfb.h"
#include "port/niagara.h"
#include "port/ski.h"
#include "fb.h"

#define NAME       "fb"
#define NAMESPACE  "hid"

#define TICK_INTERVAL  250000

static LIST_INITIALIZE(fbdevs);

fbdev_t *fbdev_register(fbdev_ops_t *ops, void *data)
{
	sysarg_t index = 0;
	
	if (!list_empty(&fbdevs)) {
		list_foreach(fbdevs, link) {
			fbdev_t *dev = list_get_instance(link, fbdev_t, link);
			if (index <= dev->index)
				index = dev->index + 1;
		}
	}
	
	fbdev_t *dev = (fbdev_t *) malloc(sizeof(fbdev_t));
	if (dev == NULL)
		return NULL;
	
	link_initialize(&dev->link);
	atomic_set(&dev->refcnt, 0);
	dev->claimed = false;
	dev->index = index;
	list_initialize(&dev->vps);
	list_initialize(&dev->frontbufs);
	list_initialize(&dev->imagemaps);
	list_initialize(&dev->sequences);
	
	dev->ops = *ops;
	dev->data = data;
	
	char node[LOC_NAME_MAXLEN + 1];
	snprintf(node, LOC_NAME_MAXLEN, "%s/%s%" PRIun, NAMESPACE, NAME,
	    index);
	
	if (loc_service_register(node, &dev->dsid) != EOK) {
		printf("%s: Unable to register device %s\n", NAME, node);
		free(dev);
		return NULL;
	}
	
	list_append(&dev->link, &fbdevs);
	return dev;
}

static void fbsrv_yield(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	assert(dev->ops.yield);
	
	if (dev->claimed) {
		int rc = dev->ops.yield(dev);
		if (rc == EOK)
			dev->claimed = false;
		
		async_answer_0(iid, rc);
	} else
		async_answer_0(iid, ENOENT);
}

static void fbsrv_claim(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	assert(dev->ops.claim);
	
	if (!dev->claimed) {
		int rc = dev->ops.claim(dev);
		if (rc == EOK)
			dev->claimed = true;
		
		async_answer_0(iid, rc);
	} else
		async_answer_0(iid, ENOENT);
}

static void fbsrv_get_resolution(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	assert(dev->ops.get_resolution);
	
	sysarg_t width;
	sysarg_t height;
	int rc = dev->ops.get_resolution(dev, &width, &height);
	
	async_answer_2(iid, rc, width, height);
}

static void fbsrv_pointer_update(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	if ((dev->claimed) && (dev->ops.pointer_update)) {
		dev->ops.pointer_update(dev, IPC_GET_ARG1(*icall),
		    IPC_GET_ARG2(*icall), IPC_GET_ARG3(*icall));
		async_answer_0(iid, EOK);
	} else
		async_answer_0(iid, ENOTSUP);
}

static fbvp_t *resolve_vp(fbdev_t *dev, sysarg_t handle, ipc_callid_t iid)
{
	fbvp_t *vp = NULL;
	list_foreach(dev->vps, link) {
		fbvp_t *cur = list_get_instance(link, fbvp_t, link);
		if (cur == (fbvp_t *) handle) {
			vp = cur;
			break;
		}
	}
	
	if (vp == NULL) {
		async_answer_0(iid, ENOENT);
		return NULL;
	}
	
	return vp;
}

static frontbuf_t *resolve_frontbuf(fbdev_t *dev, sysarg_t handle, ipc_callid_t iid)
{
	frontbuf_t *frontbuf = NULL;
	list_foreach(dev->frontbufs, link) {
		frontbuf_t *cur = list_get_instance(link, frontbuf_t, link);
		if (cur == (frontbuf_t *) handle) {
			frontbuf = cur;
			break;
		}
	}
	
	if (frontbuf == NULL) {
		async_answer_0(iid, ENOENT);
		return NULL;
	}
	
	return frontbuf;
}

static imagemap_t *resolve_imagemap(fbdev_t *dev, sysarg_t handle, ipc_callid_t iid)
{
	imagemap_t *imagemap = NULL;
	list_foreach(dev->imagemaps, link) {
		imagemap_t *cur = list_get_instance(link, imagemap_t, link);
		if (cur == (imagemap_t *) handle) {
			imagemap = cur;
			break;
		}
	}
	
	if (imagemap == NULL) {
		async_answer_0(iid, ENOENT);
		return NULL;
	}
	
	return imagemap;
}

static sequence_t *resolve_sequence(fbdev_t *dev, sysarg_t handle, ipc_callid_t iid)
{
	sequence_t *sequence = NULL;
	list_foreach(dev->sequences, link) {
		sequence_t *cur = list_get_instance(link, sequence_t, link);
		if (cur == (sequence_t *) handle) {
			sequence = cur;
			break;
		}
	}
	
	if (sequence == NULL) {
		async_answer_0(iid, ENOENT);
		return NULL;
	}
	
	return sequence;
}

static void fbsrv_vp_create(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	assert(dev->ops.font_metrics);
	assert(dev->ops.vp_create);
	
	fbvp_t *vp = (fbvp_t *) malloc(sizeof(fbvp_t));
	if (vp == NULL) {
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	link_initialize(&vp->link);
	
	vp->x = IPC_GET_ARG1(*icall);
	vp->y = IPC_GET_ARG2(*icall);
	vp->width = IPC_GET_ARG3(*icall);
	vp->height = IPC_GET_ARG4(*icall);
	
	dev->ops.font_metrics(dev, vp->width, vp->height, &vp->cols, &vp->rows);
	
	vp->cursor_active = false;
	vp->cursor_flash = false;
	
	list_initialize(&vp->sequences);
	
	vp->backbuf = screenbuffer_create(vp->cols, vp->rows,
	    SCREENBUFFER_FLAG_NONE);
	if (vp->backbuf == NULL) {
		free(vp);
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	vp->top_row = 0;
	
	int rc = dev->ops.vp_create(dev, vp);
	if (rc != EOK) {
		free(vp);
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	list_append(&vp->link, &dev->vps);
	async_answer_1(iid, EOK, (sysarg_t) vp);
}

static void fbsrv_vp_destroy(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	assert(dev->ops.vp_destroy);
	
	fbvp_t *vp = resolve_vp(dev, IPC_GET_ARG1(*icall), iid);
	if (vp == NULL)
		return;
	
	if (dev->active_vp == vp) {
		async_answer_0(iid, EPERM);
		return;
	}
	
	dev->ops.vp_destroy(dev, vp);
	
	list_remove(&vp->link);
	free(vp->backbuf);
	free(vp);
	
	async_answer_0(iid, EOK);
}

static void fbsrv_frontbuf_create(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	frontbuf_t *frontbuf = (frontbuf_t *) malloc(sizeof(frontbuf_t));
	if (frontbuf == NULL) {
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	link_initialize(&frontbuf->link);
	
	ipc_callid_t callid;
	if (!async_share_out_receive(&callid, &frontbuf->size,
	    &frontbuf->flags)) {
		free(frontbuf);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	int rc = async_share_out_finalize(callid, &frontbuf->data);
	if ((rc != EOK) || (frontbuf->data == AS_MAP_FAILED)) {
		free(frontbuf);
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	list_append(&frontbuf->link, &dev->frontbufs);
	async_answer_1(iid, EOK, (sysarg_t) frontbuf);
}

static void fbsrv_frontbuf_destroy(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	frontbuf_t *frontbuf = resolve_frontbuf(dev, IPC_GET_ARG1(*icall), iid);
	if (frontbuf == NULL)
		return;
	
	list_remove(&frontbuf->link);
	as_area_destroy(frontbuf->data);
	free(frontbuf);
	
	async_answer_0(iid, EOK);
}

static void fbsrv_imagemap_create(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	imagemap_t *imagemap = (imagemap_t *) malloc(sizeof(imagemap_t));
	if (imagemap == NULL) {
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	link_initialize(&imagemap->link);
	link_initialize(&imagemap->seq_link);
	
	ipc_callid_t callid;
	if (!async_share_out_receive(&callid, &imagemap->size,
	    &imagemap->flags)) {
		free(imagemap);
		async_answer_0(iid, EINVAL);
		return;
	}
	
	int rc = async_share_out_finalize(callid, &imagemap->data);
	if ((rc != EOK) || (imagemap->data == AS_MAP_FAILED)) {
		free(imagemap);
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	list_append(&imagemap->link, &dev->imagemaps);
	async_answer_1(iid, EOK, (sysarg_t) imagemap);
}

static void fbsrv_imagemap_destroy(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	imagemap_t *imagemap = resolve_imagemap(dev, IPC_GET_ARG1(*icall), iid);
	if (imagemap == NULL)
		return;
	
	list_remove(&imagemap->link);
	list_remove(&imagemap->seq_link);
	as_area_destroy(imagemap->data);
	free(imagemap);
	
	async_answer_0(iid, EOK);
}

static void fbsrv_sequence_create(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	sequence_t *sequence = (sequence_t *) malloc(sizeof(sequence_t));
	if (sequence == NULL) {
		async_answer_0(iid, ENOMEM);
		return;
	}
	
	link_initialize(&sequence->link);
	list_initialize(&sequence->imagemaps);
	sequence->count = 0;
	
	list_append(&sequence->link, &dev->sequences);
	async_answer_1(iid, EOK, (sysarg_t) sequence);
}

static void fbsrv_sequence_destroy(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	sequence_t *sequence = resolve_sequence(dev, IPC_GET_ARG1(*icall), iid);
	if (sequence == NULL)
		return;
	
	list_remove(&sequence->link);
	free(sequence);
	
	async_answer_0(iid, EOK);
}

static void fbsrv_sequence_add_imagemap(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	sequence_t *sequence = resolve_sequence(dev, IPC_GET_ARG1(*icall), iid);
	if (sequence == NULL)
		return;
	
	imagemap_t *imagemap = resolve_imagemap(dev, IPC_GET_ARG2(*icall), iid);
	if (imagemap == NULL)
		return;
	
	if (list_member(&imagemap->seq_link, &sequence->imagemaps)) {
		async_answer_0(iid, EEXISTS);
		return;
	}
	
	list_append(&imagemap->seq_link, &sequence->imagemaps);
	sequence->count++;
	
	async_answer_0(iid, EOK);
}

static void fbsrv_vp_focus(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	fbvp_t *vp = resolve_vp(dev, IPC_GET_ARG1(*icall), iid);
	if (vp == NULL)
		return;
	
	if (dev->active_vp != vp)
		dev->active_vp = vp;
	
	async_answer_0(iid, EOK);
}

static void fbsrv_vp_clear(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	assert(dev->ops.vp_clear);
	
	if ((dev->claimed) && (dev->active_vp)) {
		screenbuffer_set_cursor_visibility(dev->active_vp->backbuf, false);
		dev->ops.vp_clear(dev, dev->active_vp);
		async_answer_0(iid, EOK);
	} else
		async_answer_0(iid, ENOENT);
}

static void fbsrv_vp_get_dimensions(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	if (dev->active_vp)
		async_answer_2(iid, EOK, dev->active_vp->cols, dev->active_vp->rows);
	else
		async_answer_0(iid, ENOENT);
}

static void fbsrv_vp_get_caps(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	assert(dev->ops.vp_get_caps);
	
	if (dev->active_vp)
		async_answer_1(iid, EOK,
		    (sysarg_t) dev->ops.vp_get_caps(dev, dev->active_vp));
	else
		async_answer_0(iid, ENOENT);
}

static void fbsrv_vp_cursor_update(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	assert(dev->ops.vp_cursor_update);
	
	frontbuf_t *frontbuf = resolve_frontbuf(dev, IPC_GET_ARG1(*icall), iid);
	if (frontbuf == NULL)
		return;
	
	if ((dev->claimed) && (dev->active_vp)) {
		screenbuffer_t *screenbuf = (screenbuffer_t *) frontbuf->data;
		
		sysarg_t prev_col;
		sysarg_t prev_row;
		sysarg_t col;
		sysarg_t row;
		
		screenbuffer_get_cursor(dev->active_vp->backbuf,
		    &prev_col, &prev_row);
		screenbuffer_get_cursor(screenbuf, &col, &row);
		screenbuffer_set_cursor(dev->active_vp->backbuf, col, row);
		
		bool visible = screenbuffer_get_cursor_visibility(screenbuf);
		screenbuffer_set_cursor_visibility(dev->active_vp->backbuf, visible);
		
		if (visible)
			dev->active_vp->cursor_active = true;
		
		dev->ops.vp_cursor_update(dev, dev->active_vp, prev_col, prev_row,
		    col, row, visible);
		async_answer_0(iid, EOK);
	} else
		async_answer_0(iid, ENOENT);
}

static void fbsrv_vp_cursor_flash(fbdev_t *dev)
{
	if ((dev->claimed) && (dev->ops.vp_cursor_flash)) {
		list_foreach (dev->vps, link) {
			fbvp_t *vp = list_get_instance(link, fbvp_t, link);
			
			if (vp->cursor_active) {
				sysarg_t col;
				sysarg_t row;
				
				screenbuffer_get_cursor(vp->backbuf, &col, &row);
				vp->cursor_flash = !vp->cursor_flash;
				dev->ops.vp_cursor_flash(dev, vp, col, row);
			}
		}
	}
}

static void fbsrv_sequences_update(fbdev_t *dev)
{
	if ((dev->claimed) && (dev->ops.vp_imgmap_damage)) {
		list_foreach (dev->vps, vp_link) {
			fbvp_t *vp = list_get_instance(vp_link, fbvp_t, link);
			
			list_foreach (vp->sequences, seq_vp_link) {
				sequence_vp_t *seq_vp =
				    list_get_instance(seq_vp_link, sequence_vp_t, link);
				
				seq_vp->current++;
				if (seq_vp->current >= seq_vp->seq->count)
					seq_vp->current = 0;
				
				link_t *link =
				    list_nth(&seq_vp->seq->imagemaps, seq_vp->current);
				if (link != NULL) {
					imagemap_t *imagemap =
					    list_get_instance(link, imagemap_t, seq_link);
					
					imgmap_t *imgmap = (imgmap_t *) imagemap->data;
					sysarg_t width;
					sysarg_t height;
					
					imgmap_get_resolution(imgmap, &width, &height);
					dev->ops.vp_imgmap_damage(dev, vp, imgmap,
					    0, 0, width, height);
				}
			}
		}
	}
}

static void fbsrv_vp_set_style(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	if (dev->active_vp) {
		dev->active_vp->attrs.type = CHAR_ATTR_STYLE;
		dev->active_vp->attrs.val.style =
		    (console_style_t) IPC_GET_ARG1(*icall);
		async_answer_0(iid, EOK);
	} else
		async_answer_0(iid, ENOENT);
}

static void fbsrv_vp_set_color(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	if (dev->active_vp) {
		dev->active_vp->attrs.type = CHAR_ATTR_INDEX;
		dev->active_vp->attrs.val.index.bgcolor =
		    (console_color_t) IPC_GET_ARG1(*icall);
		dev->active_vp->attrs.val.index.fgcolor =
		    (console_color_t) IPC_GET_ARG2(*icall);
		dev->active_vp->attrs.val.index.attr =
		    (console_color_attr_t) IPC_GET_ARG3(*icall);
		async_answer_0(iid, EOK);
	} else
		async_answer_0(iid, ENOENT);
}

static void fbsrv_vp_set_rgb_color(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	if (dev->active_vp) {
		dev->active_vp->attrs.type = CHAR_ATTR_RGB;
		dev->active_vp->attrs.val.rgb.bgcolor = IPC_GET_ARG1(*icall);
		dev->active_vp->attrs.val.rgb.fgcolor = IPC_GET_ARG2(*icall);
		async_answer_0(iid, EOK);
	} else
		async_answer_0(iid, ENOENT);
}

static void fbsrv_vp_putchar(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	assert(dev->ops.vp_char_update);
	
	if ((dev->claimed) && (dev->active_vp)) {
		charfield_t *field = screenbuffer_field_at(dev->active_vp->backbuf,
		    IPC_GET_ARG1(*icall), IPC_GET_ARG2(*icall));
		
		field->ch = IPC_GET_ARG3(*icall);
		
		dev->ops.vp_char_update(dev, dev->active_vp, IPC_GET_ARG1(*icall),
		    IPC_GET_ARG2(*icall));
		async_answer_0(iid, EOK);
	} else
		async_answer_0(iid, ENOENT);
}

static bool fbsrv_vp_update_scroll(fbdev_t *dev, fbvp_t *vp,
    screenbuffer_t *frontbuf)
{
	assert(dev->ops.vp_char_update);
	
	sysarg_t top_row = screenbuffer_get_top_row(frontbuf);
	
	if (vp->top_row == top_row)
		return false;
	
	vp->top_row = top_row;
	
	for (sysarg_t y = 0; y < vp->rows; y++) {
		for (sysarg_t x = 0; x < vp->cols; x++) {
			charfield_t *front_field =
			    screenbuffer_field_at(frontbuf, x, y);
			charfield_t *back_field =
			    screenbuffer_field_at(vp->backbuf, x, y);
			bool update = false;
			
			if (front_field->ch != back_field->ch) {
				back_field->ch = front_field->ch;
				update = true;
			}
			
			if (!attrs_same(front_field->attrs, back_field->attrs)) {
				back_field->attrs = front_field->attrs;
				update = true;
			}
			
			front_field->flags &= ~CHAR_FLAG_DIRTY;
			
			if (update)
				dev->ops.vp_char_update(dev, vp, x, y);
		}
	}
	
	return true;
}

static void fbsrv_vp_update(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	assert(dev->ops.vp_char_update);
	
	frontbuf_t *frontbuf = resolve_frontbuf(dev, IPC_GET_ARG1(*icall), iid);
	if (frontbuf == NULL)
		return;
	
	if ((dev->claimed) && (dev->active_vp)) {
		fbvp_t *vp = dev->active_vp;
		screenbuffer_t *screenbuf = (screenbuffer_t *) frontbuf->data;
		
		if (fbsrv_vp_update_scroll(dev, vp, screenbuf)) {
			async_answer_0(iid, EOK);
			return;
		}
		
		for (sysarg_t y = 0; y < vp->rows; y++) {
			for (sysarg_t x = 0; x < vp->cols; x++) {
				charfield_t *front_field =
				    screenbuffer_field_at(screenbuf, x, y);
				charfield_t *back_field =
				    screenbuffer_field_at(vp->backbuf, x, y);
				bool update = false;
				
				if ((front_field->flags & CHAR_FLAG_DIRTY) == CHAR_FLAG_DIRTY) {
					if (front_field->ch != back_field->ch) {
						back_field->ch = front_field->ch;
						update = true;
					}
					
					if (!attrs_same(front_field->attrs, back_field->attrs)) {
						back_field->attrs = front_field->attrs;
						update = true;
					}
					
					front_field->flags &= ~CHAR_FLAG_DIRTY;
				}
				
				if (update)
					dev->ops.vp_char_update(dev, vp, x, y);
			}
		}
		
		async_answer_0(iid, EOK);
	} else
		async_answer_0(iid, ENOENT);
}

static void fbsrv_vp_damage(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	assert(dev->ops.vp_char_update);
	
	frontbuf_t *frontbuf = resolve_frontbuf(dev, IPC_GET_ARG1(*icall), iid);
	if (frontbuf == NULL)
		return;
	
	if ((dev->claimed) && (dev->active_vp)) {
		fbvp_t *vp = dev->active_vp;
		screenbuffer_t *screenbuf = (screenbuffer_t *) frontbuf->data;
		
		if (fbsrv_vp_update_scroll(dev, vp, screenbuf)) {
			async_answer_0(iid, EOK);
			return;
		}
		
		sysarg_t col = IPC_GET_ARG2(*icall);
		sysarg_t row = IPC_GET_ARG3(*icall);
		
		sysarg_t cols = IPC_GET_ARG4(*icall);
		sysarg_t rows = IPC_GET_ARG5(*icall);
		
		for (sysarg_t y = 0; y < rows; y++) {
			for (sysarg_t x = 0; x < cols; x++) {
				charfield_t *front_field =
				    screenbuffer_field_at(screenbuf, col + x, row + y);
				charfield_t *back_field =
				    screenbuffer_field_at(vp->backbuf, col + x, row + y);
				bool update = false;
				
				if (front_field->ch != back_field->ch) {
					back_field->ch = front_field->ch;
					update = true;
				}
				
				if (!attrs_same(front_field->attrs, back_field->attrs)) {
					back_field->attrs = front_field->attrs;
					update = true;
				}
				
				front_field->flags &= ~CHAR_FLAG_DIRTY;
				
				if (update)
					dev->ops.vp_char_update(dev, vp, col + x, row + y);
			}
		}
		
		async_answer_0(iid, EOK);
	} else
		async_answer_0(iid, ENOENT);
}

static void fbsrv_vp_imagemap_damage(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	imagemap_t *imagemap = resolve_imagemap(dev, IPC_GET_ARG1(*icall), iid);
	if (imagemap == NULL)
		return;
	
	if ((dev->claimed) && (dev->ops.vp_imgmap_damage)) {
		if (dev->active_vp) {
			dev->ops.vp_imgmap_damage(dev, dev->active_vp,
			    (imgmap_t *) imagemap->data,
			    IPC_GET_ARG2(*icall), IPC_GET_ARG3(*icall),
			    IPC_GET_ARG4(*icall), IPC_GET_ARG5(*icall));
			async_answer_0(iid, EOK);
		} else
			async_answer_0(iid, ENOENT);
	} else
		async_answer_0(iid, ENOTSUP);
}

static void fbsrv_vp_sequence_start(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	sequence_t *sequence = resolve_sequence(dev, IPC_GET_ARG1(*icall), iid);
	if (sequence == NULL)
		return;
	
	if (dev->active_vp) {
		/* Check if the sequence is not already started */
		list_foreach(dev->active_vp->sequences, link) {
			sequence_vp_t *seq_vp =
			    list_get_instance(link, sequence_vp_t, link);
			
			if (seq_vp->seq == sequence) {
				async_answer_0(iid, EEXISTS);
				return;
			}
		}
		
		sequence_vp_t *seq_vp =
		    (sequence_vp_t *) malloc(sizeof(sequence_vp_t));
		
		if (seq_vp != NULL) {
			link_initialize(&seq_vp->link);
			seq_vp->seq = sequence;
			seq_vp->current = 0;
			
			list_append(&seq_vp->link, &dev->active_vp->sequences);
			async_answer_0(iid, EOK);
		} else
			async_answer_0(iid, ENOMEM);
	} else
		async_answer_0(iid, ENOENT);
}

static void fbsrv_vp_sequence_stop(fbdev_t *dev, ipc_callid_t iid, ipc_call_t *icall)
{
	sequence_t *sequence = resolve_sequence(dev, IPC_GET_ARG1(*icall), iid);
	if (sequence == NULL)
		return;
	
	if (dev->active_vp) {
		list_foreach(dev->active_vp->sequences, link) {
			sequence_vp_t *seq_vp =
			    list_get_instance(link, sequence_vp_t, link);
			
			if (seq_vp->seq == sequence) {
				list_remove(&seq_vp->link);
				free(seq_vp);
				
				async_answer_0(iid, EOK);
				return;
			}
		}
		
		async_answer_0(iid, ENOENT);
	} else
		async_answer_0(iid, ENOENT);
}

static void client_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	fbdev_t *dev = NULL;
	service_id_t dsid = (service_id_t) IPC_GET_ARG1(*icall);
	
	list_foreach(fbdevs, link) {
		fbdev_t *fbdev = list_get_instance(link, fbdev_t, link);
		if (fbdev->dsid == dsid) {
			dev = fbdev;
			break;
		}
	}
	
	if (dev == NULL) {
		async_answer_0(iid, ENOENT);
		return;
	}
	
	if (atomic_get(&dev->refcnt) > 0) {
		async_answer_0(iid, ELIMIT);
		return;
	}
	
	/*
	 * Accept the connection
	 */
	
	atomic_inc(&dev->refcnt);
	dev->claimed = true;
	async_answer_0(iid, EOK);
	
	while (true) {
		ipc_call_t call;
		ipc_callid_t callid =
		    async_get_call_timeout(&call, TICK_INTERVAL);
		
		if (!callid) {
			fbsrv_vp_cursor_flash(dev);
			fbsrv_sequences_update(dev);
			continue;
		}
		
		if (!IPC_GET_IMETHOD(call)) {
			dev->claimed = false;
			atomic_dec(&dev->refcnt);
			async_answer_0(callid, EOK);
			break;
		}
		
		switch (IPC_GET_IMETHOD(call)) {
			
			/* Screen methods */
			
			case FB_GET_RESOLUTION:
				fbsrv_get_resolution(dev, callid, &call);
				break;
			case FB_YIELD:
				fbsrv_yield(dev, callid, &call);
				break;
			case FB_CLAIM:
				fbsrv_claim(dev, callid, &call);
				break;
			case FB_POINTER_UPDATE:
				fbsrv_pointer_update(dev, callid, &call);
				break;
			
			/* Object methods */
			
			case FB_VP_CREATE:
				fbsrv_vp_create(dev, callid, &call);
				break;
			case FB_VP_DESTROY:
				fbsrv_vp_destroy(dev, callid, &call);
				break;
			case FB_FRONTBUF_CREATE:
				fbsrv_frontbuf_create(dev, callid, &call);
				break;
			case FB_FRONTBUF_DESTROY:
				fbsrv_frontbuf_destroy(dev, callid, &call);
				break;
			case FB_IMAGEMAP_CREATE:
				fbsrv_imagemap_create(dev, callid, &call);
				break;
			case FB_IMAGEMAP_DESTROY:
				fbsrv_imagemap_destroy(dev, callid, &call);
				break;
			case FB_SEQUENCE_CREATE:
				fbsrv_sequence_create(dev, callid, &call);
				break;
			case FB_SEQUENCE_DESTROY:
				fbsrv_sequence_destroy(dev, callid, &call);
				break;
			case FB_SEQUENCE_ADD_IMAGEMAP:
				fbsrv_sequence_add_imagemap(dev, callid, &call);
				break;
			
			/* Viewport stateful methods */
			
			case FB_VP_FOCUS:
				fbsrv_vp_focus(dev, callid, &call);
				break;
			case FB_VP_CLEAR:
				fbsrv_vp_clear(dev, callid, &call);
				break;
			case FB_VP_GET_DIMENSIONS:
				fbsrv_vp_get_dimensions(dev, callid, &call);
				break;
			case FB_VP_GET_CAPS:
				fbsrv_vp_get_caps(dev, callid, &call);
				break;
			
			/* Style methods (viewport specific) */
			
			case FB_VP_CURSOR_UPDATE:
				fbsrv_vp_cursor_update(dev, callid, &call);
				break;
			case FB_VP_SET_STYLE:
				fbsrv_vp_set_style(dev, callid, &call);
				break;
			case FB_VP_SET_COLOR:
				fbsrv_vp_set_color(dev, callid, &call);
				break;
			case FB_VP_SET_RGB_COLOR:
				fbsrv_vp_set_rgb_color(dev, callid, &call);
				break;
			
			/* Text output methods (viewport specific) */
			
			case FB_VP_PUTCHAR:
				fbsrv_vp_putchar(dev, callid, &call);
				break;
			case FB_VP_UPDATE:
				fbsrv_vp_update(dev, callid, &call);
				break;
			case FB_VP_DAMAGE:
				fbsrv_vp_damage(dev, callid, &call);
				break;
			
			/* Image map methods (viewport specific) */
			
			case FB_VP_IMAGEMAP_DAMAGE:
				fbsrv_vp_imagemap_damage(dev, callid, &call);
				break;
			
			/* Sequence methods (viewport specific) */
			
			case FB_VP_SEQUENCE_START:
				fbsrv_vp_sequence_start(dev, callid, &call);
				break;
			case FB_VP_SEQUENCE_STOP:
				fbsrv_vp_sequence_stop(dev, callid, &call);
				break;
			
			default:
				async_answer_0(callid, EINVAL);
		}
	}
}

int main(int argc, char *argv[])
{
	printf("%s: HelenOS framebuffer service\n", NAME);
	
	/* Register server */
	async_set_client_connection(client_connection);
	int rc = loc_server_register(NAME);
	if (rc != EOK) {
		printf("%s: Unable to register driver\n", NAME);
		return rc;
	}
	
	ega_init();
	kchar_init();
	kfb_init();
	niagara_init();
	ski_init();
	
	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();
	
	/* Never reached */
	return 0;
}
