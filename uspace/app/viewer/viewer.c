/*
 * Copyright (c) 2020 Jiri Svoboda
 * Copyright (c) 2013 Martin Decky
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

/** @addtogroup viewer
 * @{
 */
/** @file
 */

#include <draw/surface.h>
#include <draw/codec.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <ui/image.h>
#include <ui/ui.h>
#include <ui/wdecor.h>
#include <ui/window.h>
#include <vfs/vfs.h>

#define NAME  "viewer"

typedef struct {
	ui_t *ui;
} viewer_t;

static size_t imgs_count;
static size_t imgs_current = 0;
static char **imgs;

static ui_window_t *window;
static surface_t *surface = NULL;
static ui_image_t *image = NULL;
static gfx_context_t *window_gc;

static surface_coord_t img_width;
static surface_coord_t img_height;

static bool img_load(const char *, surface_t **);
static bool img_setup(gfx_context_t *, surface_t *);

static void wnd_close(ui_window_t *, void *);
static void wnd_kbd_event(ui_window_t *, void *, kbd_event_t *);

static ui_window_cb_t window_cb = {
	.close = wnd_close,
	.kbd = wnd_kbd_event
};

/** Window close request
 *
 * @param window Window
 * @param arg Argument (calc_t *)
 */
static void wnd_close(ui_window_t *window, void *arg)
{
	viewer_t *viewer = (viewer_t *) arg;

	ui_quit(viewer->ui);
}

static void wnd_kbd_event(ui_window_t *window, void *arg,
    kbd_event_t *event)
{
	bool update = false;

	if ((event->type == KEY_PRESS) && (event->c == 'q'))
		exit(0);

	if ((event->type == KEY_PRESS) && (event->key == KC_PAGE_DOWN)) {
		if (imgs_current == imgs_count - 1)
			imgs_current = 0;
		else
			imgs_current++;

		update = true;
	}

	if ((event->type == KEY_PRESS) && (event->key == KC_PAGE_UP)) {
		if (imgs_current == 0)
			imgs_current = imgs_count - 1;
		else
			imgs_current--;

		update = true;
	}

	if (update) {
		surface_t *lsface;

		if (!img_load(imgs[imgs_current], &lsface)) {
			printf("Cannot load image \"%s\".\n", imgs[imgs_current]);
			exit(4);
		}
		if (!img_setup(window_gc, lsface)) {
			printf("Cannot setup image \"%s\".\n", imgs[imgs_current]);
			exit(6);
		}
	}
}

static bool img_load(const char *fname, surface_t **p_local_surface)
{
	int fd;
	errno_t rc = vfs_lookup_open(fname, WALK_REGULAR, MODE_READ, &fd);
	if (rc != EOK)
		return false;

	vfs_stat_t stat;
	rc = vfs_stat(fd, &stat);
	if (rc != EOK) {
		vfs_put(fd);
		return false;
	}

	void *tga = malloc(stat.size);
	if (tga == NULL) {
		vfs_put(fd);
		return false;
	}

	size_t nread;
	rc = vfs_read(fd, (aoff64_t []) { 0 }, tga, stat.size, &nread);
	if (rc != EOK || nread != stat.size) {
		free(tga);
		vfs_put(fd);
		return false;
	}

	vfs_put(fd);

	*p_local_surface = decode_tga(tga, stat.size, SURFACE_FLAG_SHARED);
	if (*p_local_surface == NULL) {
		free(tga);
		return false;
	}

	free(tga);

	surface_get_resolution(*p_local_surface, &img_width, &img_height);

	return true;
}

static bool img_setup(gfx_context_t *gc, surface_t *local_surface)
{
	surface_coord_t w, h;
	gfx_bitmap_params_t params;
	gfx_bitmap_alloc_t alloc;
	gfx_bitmap_t *bmp;
	gfx_rect_t arect;
	gfx_rect_t irect;
	ui_resource_t *ui_res;
	errno_t rc;

	ui_res = ui_window_get_res(window);

	surface_get_resolution(local_surface, &w, &h);
	gfx_bitmap_params_init(&params);
	params.rect.p1.x = w;
	params.rect.p1.y = h;

	ui_window_get_app_rect(window, &arect);
	gfx_rect_translate(&arect.p0, &params.rect, &irect);

	alloc.pitch = sizeof(uint32_t) * w;
	alloc.off0 = 0;
	alloc.pixels = surface_direct_access(local_surface);

	rc = gfx_bitmap_create(gc, &params, &alloc, &bmp);
	if (rc != EOK) {
		surface_destroy(local_surface);
		return false;
	}

	if (image != NULL) {
		ui_image_set_bmp(image, bmp, &params.rect);
		(void) ui_image_paint(image);
		ui_image_set_rect(image, &irect);
	} else {
		rc = ui_image_create(ui_res, bmp, &params.rect, &image);
		if (rc != EOK) {
			gfx_bitmap_destroy(bmp);
			surface_destroy(local_surface);
			return false;
		}

		ui_image_set_rect(image, &irect);
		ui_window_add(window, ui_image_ctl(image));
	}

	if (surface != NULL)
		surface_destroy(surface);

	surface = local_surface;
	return true;
}

static void print_syntax(void)
{
	printf("Syntax: %s [<options] <image-file>...\n", NAME);
	printf("\t-d <display-spec> Use the specified display\n");
	printf("\t-f                Full-screen mode\n");
}

int main(int argc, char *argv[])
{
	const char *display_spec = DISPLAY_DEFAULT;
	surface_t *lsface;
	bool fullscreen = false;
	gfx_rect_t rect;
	gfx_rect_t wrect;
	gfx_coord2_t off;
	ui_t *ui;
	ui_wnd_params_t params;
	viewer_t viewer;
	errno_t rc;
	int i;

	i = 1;
	while (i < argc && argv[i][0] == '-') {
		if (str_cmp(argv[i], "-d") == 0) {
			++i;
			if (i >= argc) {
				printf("Argument missing.\n");
				print_syntax();
				return 1;
			}

			display_spec = argv[i++];
		} else if (str_cmp(argv[i], "-f") == 0) {
			fullscreen = true;
		} else {
			printf("Invalid option '%s'.\n", argv[i]);
			print_syntax();
			return 1;
		}
	}

	if (i >= argc) {
		printf("No image files specified.\n");
		print_syntax();
		return 1;
	}

	imgs_count = argc - i;
	imgs = calloc(imgs_count, sizeof(char *));
	if (imgs == NULL) {
		printf("Out of memory.\n");
		return 1;
	}

	for (int j = 0; j < argc - i; j++) {
		imgs[j] = str_dup(argv[i + j]);
		if (imgs[j] == NULL) {
			printf("Out of memory.\n");
			return 3;
		}
	}

	if (!img_load(imgs[imgs_current], &lsface)) {
		printf("Cannot load image \"%s\".\n", imgs[imgs_current]);
		return 1;
	}

	// TODO Fullscreen mode
	if (fullscreen) {
		printf("Fullscreen mode not implemented.\n");
		return 1;
	}

	rc = ui_create(display_spec, &ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		return 1;
	}

	viewer.ui = ui;

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = img_width;
	rect.p1.y = img_height;

	ui_wnd_params_init(&params);
	params.caption = "Viewer";
	/*
	 * Compute window rectangle such that application area corresponds
	 * to rect
	 */
	ui_wdecor_rect_from_app(&rect, &wrect);
	off = wrect.p0;
	gfx_rect_rtranslate(&off, &wrect, &params.rect);

	rc = ui_window_create(ui, &params, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		return 1;
	}

	window_gc = ui_window_get_gc(window);

	ui_window_set_cb(window, &window_cb, (void *) &viewer);

	if (!img_setup(window_gc, lsface)) {
		printf("Cannot setup image \"%s\".\n", imgs[imgs_current]);
		return 1;
	}

	rc = ui_window_paint(window);
	if (rc != EOK) {
		printf("Error painting window.\n");
		return 1;
	}

	ui_run(ui);

	return 0;
}

/** @}
 */
