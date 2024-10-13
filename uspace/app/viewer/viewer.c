/*
 * Copyright (c) 2024 Jiri Svoboda
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

#include <errno.h>
#include <gfximage/tga.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <ui/filedialog.h>
#include <ui/image.h>
#include <ui/ui.h>
#include <ui/wdecor.h>
#include <ui/window.h>
#include <vfs/vfs.h>

#define NAME  "viewer"

typedef struct {
	ui_t *ui;

	size_t imgs_count;
	size_t imgs_current;
	char **imgs;

	bool fullscreen;
	ui_window_t *window;
	gfx_bitmap_t *bitmap;
	ui_image_t *image;
	gfx_context_t *window_gc;
	ui_file_dialog_t *dialog;

	gfx_rect_t img_rect;
} viewer_t;

static bool viewer_img_load(viewer_t *, const char *, gfx_bitmap_t **,
    gfx_rect_t *);
static bool viewer_img_setup(viewer_t *, gfx_bitmap_t *, gfx_rect_t *);
static errno_t viewer_window_create(viewer_t *);
static void viewer_window_destroy(viewer_t *);

static void wnd_close(ui_window_t *, void *);
static void wnd_kbd_event(ui_window_t *, void *, kbd_event_t *);

static ui_window_cb_t window_cb = {
	.close = wnd_close,
	.kbd = wnd_kbd_event
};

static void file_dialog_bok(ui_file_dialog_t *, void *, const char *);
static void file_dialog_bcancel(ui_file_dialog_t *, void *);
static void file_dialog_close(ui_file_dialog_t *, void *);

static ui_file_dialog_cb_t file_dialog_cb = {
	.bok = file_dialog_bok,
	.bcancel = file_dialog_bcancel,
	.close = file_dialog_close
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

/** Viewer unmodified key press.
 *
 * @param viewer Viewer
 * @param event Keyboard event
 */
static void viewer_kbd_event_unmod(viewer_t *viewer, kbd_event_t *event)
{
	bool update = false;

	if (event->key == KC_Q || event->key == KC_ESCAPE)
		ui_quit(viewer->ui);

	if (event->key == KC_PAGE_DOWN) {
		if (viewer->imgs_current == viewer->imgs_count - 1)
			viewer->imgs_current = 0;
		else
			viewer->imgs_current++;

		update = true;
	}

	if (event->key == KC_PAGE_UP) {
		if (viewer->imgs_current == 0)
			viewer->imgs_current = viewer->imgs_count - 1;
		else
			viewer->imgs_current--;

		update = true;
	}

	if (update) {
		gfx_bitmap_t *lbitmap;
		gfx_rect_t lrect;

		if (!viewer_img_load(viewer, viewer->imgs[viewer->imgs_current],
		    &lbitmap, &lrect)) {
			printf("Cannot load image \"%s\".\n",
			    viewer->imgs[viewer->imgs_current]);
			exit(4);
		}
		if (!viewer_img_setup(viewer, lbitmap, &lrect)) {
			printf("Cannot setup image \"%s\".\n",
			    viewer->imgs[viewer->imgs_current]);
			exit(6);
		}
	}
}

/** Viewer ctrl-key key press.
 *
 * @param viewer Viewer
 * @param event Keyboard event
 */
static void viewer_kbd_event_ctrl(viewer_t *viewer, kbd_event_t *event)
{
	if (event->key == KC_Q)
		ui_quit(viewer->ui);
}

/** Viewer window keyboard event.
 *
 * @param window UI window
 * @param arg Argument (viewer_t *)
 * @param event Keyboard event
 */
static void wnd_kbd_event(ui_window_t *window, void *arg,
    kbd_event_t *event)
{
	viewer_t *viewer = (viewer_t *)arg;

	if (event->type != KEY_PRESS)
		return;

	if ((event->mods & (KM_CTRL | KM_ALT | KM_SHIFT)) == 0)
		viewer_kbd_event_unmod(viewer, event);

	if ((event->mods & KM_CTRL) != 0 &&
	    (event->mods & (KM_ALT | KM_SHIFT)) == 0)
		viewer_kbd_event_ctrl(viewer, event);

	ui_window_def_kbd(window, event);
}

/** File dialog OK button press.
 *
 * @param dialog File dialog
 * @param arg Argument (viewer_t *)
 * @param fname File name
 */
static void file_dialog_bok(ui_file_dialog_t *dialog, void *arg,
    const char *fname)
{
	viewer_t *viewer = (viewer_t *) arg;
	errno_t rc;

	viewer->imgs_count = 1;
	viewer->imgs = calloc(viewer->imgs_count, sizeof(char *));
	if (viewer->imgs == NULL) {
		printf("Out of memory.\n");
		ui_quit(viewer->ui);
		return;
	}

	viewer->imgs[0] = str_dup(fname);
	if (viewer->imgs[0] == NULL) {
		printf("Out of memory.\n");
		ui_quit(viewer->ui);
		return;
	}

	rc = viewer_window_create(viewer);
	if (rc != EOK)
		ui_quit(viewer->ui);

	ui_file_dialog_destroy(dialog);
	viewer->dialog = NULL;
}

/** File dialog cancel button press.
 *
 * @param dialog File dialog
 * @param arg Argument (viewer_t *)
 */
static void file_dialog_bcancel(ui_file_dialog_t *dialog, void *arg)
{
	viewer_t *viewer = (viewer_t *) arg;

	ui_file_dialog_destroy(dialog);
	ui_quit(viewer->ui);
}

/** File dialog close request.
 *
 * @param dialog File dialog
 * @param arg Argument (viewer_t *)
 */
static void file_dialog_close(ui_file_dialog_t *dialog, void *arg)
{
	viewer_t *viewer = (viewer_t *) arg;

	ui_file_dialog_destroy(dialog);
	ui_quit(viewer->ui);
}

static bool viewer_img_load(viewer_t *viewer, const char *fname,
    gfx_bitmap_t **rbitmap, gfx_rect_t *rect)
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

	rc = decode_tga(viewer->window_gc, tga, stat.size, rbitmap, rect);
	if (rc != EOK) {
		free(tga);
		return false;
	}

	free(tga);

	viewer->img_rect = *rect;
	return true;
}

static bool viewer_img_setup(viewer_t *viewer, gfx_bitmap_t *bmp,
    gfx_rect_t *rect)
{
	gfx_rect_t arect;
	gfx_rect_t irect;
	ui_resource_t *ui_res;
	errno_t rc;

	ui_res = ui_window_get_res(viewer->window);

	ui_window_get_app_rect(viewer->window, &arect);

	/* Center image on application area */
	gfx_rect_ctr_on_rect(rect, &arect, &irect);

	if (viewer->image != NULL) {
		ui_image_set_bmp(viewer->image, bmp, rect);
		(void) ui_image_paint(viewer->image);
		ui_image_set_rect(viewer->image, &irect);
	} else {
		rc = ui_image_create(ui_res, bmp, rect, &viewer->image);
		if (rc != EOK) {
			gfx_bitmap_destroy(bmp);
			return false;
		}

		ui_image_set_rect(viewer->image, &irect);
		ui_window_add(viewer->window, ui_image_ctl(viewer->image));
	}

	if (viewer->bitmap != NULL)
		gfx_bitmap_destroy(viewer->bitmap);

	viewer->bitmap = bmp;
	return true;
}

static void print_syntax(void)
{
	printf("Syntax: %s [<options] <image-file>...\n", NAME);
	printf("\t-d <display-spec> Use the specified display\n");
	printf("\t-f                Full-screen mode\n");
}

static errno_t viewer_window_create(viewer_t *viewer)
{
	ui_wnd_params_t params;
	gfx_bitmap_t *lbitmap;
	gfx_rect_t lrect;
	gfx_rect_t wrect;
	gfx_coord2_t off;
	gfx_rect_t rect;
	errno_t rc;

	/*
	 * We don't know the image size yet, so create tiny window and resize
	 * later.
	 */
	ui_wnd_params_init(&params);
	params.caption = "Viewer";
	params.rect.p0.x = 0;
	params.rect.p0.y = 0;
	params.rect.p1.x = 1;
	params.rect.p1.y = 1;

	if (viewer->fullscreen) {
		params.style &= ~ui_wds_decorated;
		params.placement = ui_wnd_place_full_screen;
	}

	rc = ui_window_create(viewer->ui, &params, &viewer->window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		goto error;
	}

	viewer->window_gc = ui_window_get_gc(viewer->window);

	ui_window_set_cb(viewer->window, &window_cb, (void *)viewer);

	if (!viewer_img_load(viewer, viewer->imgs[viewer->imgs_current],
	    &lbitmap, &lrect)) {
		printf("Cannot load image \"%s\".\n",
		    viewer->imgs[viewer->imgs_current]);
		goto error;
	}

	/*
	 * Compute window rectangle such that application area corresponds
	 * to rect
	 */
	ui_wdecor_rect_from_app(viewer->ui, params.style, &lrect, &wrect);
	off = wrect.p0;
	gfx_rect_rtranslate(&off, &wrect, &rect);

	if (!viewer->fullscreen) {
		rc = ui_window_resize(viewer->window, &rect);
		if (rc != EOK) {
			printf("Error resizing window.\n");
			goto error;
		}
	}

	if (!viewer_img_setup(viewer, lbitmap, &lrect)) {
		printf("Cannot setup image \"%s\".\n",
		    viewer->imgs[viewer->imgs_current]);
		goto error;
	}

	rc = ui_window_paint(viewer->window);
	if (rc != EOK) {
		printf("Error painting window.\n");
		goto error;
	}

	return EOK;
error:
	viewer_window_destroy(viewer);
	ui_quit(viewer->ui);
	return rc;
}

static void viewer_window_destroy(viewer_t *viewer)
{
	if (viewer->window != NULL)
		ui_window_destroy(viewer->window);
	viewer->window = NULL;
}

int main(int argc, char *argv[])
{
	const char *display_spec = UI_ANY_DEFAULT;
	ui_t *ui = NULL;
	viewer_t *viewer;
	errno_t rc;
	int i;
	unsigned u;
	ui_file_dialog_params_t fdparams;

	viewer = calloc(1, sizeof(viewer_t));
	if (viewer == NULL) {
		printf("Out of memory.\n");
		goto error;
	}

	i = 1;
	while (i < argc && argv[i][0] == '-') {
		if (str_cmp(argv[i], "-d") == 0) {
			++i;
			if (i >= argc) {
				printf("Argument missing.\n");
				print_syntax();
				goto error;
			}

			display_spec = argv[i++];
		} else if (str_cmp(argv[i], "-f") == 0) {
			++i;
			viewer->fullscreen = true;
		} else {
			printf("Invalid option '%s'.\n", argv[i]);
			print_syntax();
			goto error;
		}
	}

	/* Images specified? */
	if (i < argc) {
		viewer->imgs_count = argc - i;
		viewer->imgs = calloc(viewer->imgs_count, sizeof(char *));
		if (viewer->imgs == NULL) {
			printf("Out of memory.\n");
			goto error;
		}

		for (int j = 0; j < argc - i; j++) {
			viewer->imgs[j] = str_dup(argv[i + j]);
			if (viewer->imgs[j] == NULL) {
				printf("Out of memory.\n");
				goto error;
			}
		}
	}

	rc = ui_create(display_spec, &ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		goto error;
	}

	if (ui_is_fullscreen(ui))
		viewer->fullscreen = true;

	viewer->ui = ui;

	if (viewer->imgs != NULL) {
		/* We have images, create viewer window. */
		rc = viewer_window_create(viewer);
		if (rc != EOK)
			goto error;
	} else {
		/* No images specified, browse for one. */
		ui_file_dialog_params_init(&fdparams);
		fdparams.caption = "Open Image";

		rc = ui_file_dialog_create(viewer->ui, &fdparams,
		    &viewer->dialog);
		if (rc != EOK) {
			printf("Error creating file dialog.\n");
			goto error;
		}

		ui_file_dialog_set_cb(viewer->dialog, &file_dialog_cb,
		    (void *)viewer);
	}

	ui_run(ui);

	ui_window_destroy(viewer->window);
	ui_destroy(ui);
	free(viewer);

	return 0;
error:
	if (viewer != NULL && viewer->dialog != NULL)
		ui_file_dialog_destroy(viewer->dialog);
	if (viewer != NULL && viewer->imgs != NULL) {
		for (u = 0; u < viewer->imgs_count; u++) {
			if (viewer->imgs[u] != NULL)
				free(viewer->imgs[u]);
		}
		free(viewer->imgs);
	}
	if (viewer != NULL)
		viewer_window_destroy(viewer);
	if (ui != NULL)
		ui_destroy(ui);
	if (viewer != NULL)
		free(viewer);
	return 1;
}

/** @}
 */
