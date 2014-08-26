/*
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

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <malloc.h>
#include <stdbool.h>
#include <window.h>
#include <canvas.h>
#include <surface.h>
#include <codec/tga.h>
#include <task.h>

#define NAME  "viewer"

#define WINDOW_WIDTH   1024
#define WINDOW_HEIGHT  768

static size_t imgs_count;
static size_t imgs_current = 0;
static char **imgs;

static window_t *main_window;
static surface_t *surface = NULL;
static canvas_t *canvas = NULL;

static bool img_load(const char *);

static void on_keyboard_event(widget_t *widget, void *data)
{
	kbd_event_t *event = (kbd_event_t *) data;
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
		if (!img_load(imgs[imgs_current])) {
			printf("Cannot load image \"%s\".\n", imgs[imgs_current]);
			exit(2);
		}
	}
}

static bool img_load(const char *fname)
{
	int fd = open(fname, O_RDONLY);
	if (fd < 0)
		return false;
	
	struct stat stat;
	int rc = fstat(fd, &stat);
	if (rc != EOK) {
		close(fd);
		return false;
	}
	
	void *tga = malloc(stat.size);
	if (tga == NULL) {
		close(fd);
		return false;
	}
	
	ssize_t rd = read_all(fd, tga, stat.size);
	if ((rd < 0) || (rd != (ssize_t) stat.size)) {
		free(tga);
		close(fd);
		return false;
	}
	
	close(fd);
	
	surface_t *local_surface = decode_tga(tga, stat.size, 0);
	if (local_surface == NULL) {
		free(tga);
		return false;
	}
	
	free(tga);
	
	if (canvas != NULL) {
		if (!update_canvas(canvas, local_surface)) {
			surface_destroy(local_surface);
			return false;
		}
	} else {
		canvas = create_canvas(window_root(main_window),
		    WINDOW_WIDTH, WINDOW_HEIGHT, local_surface);
		if (canvas == NULL) {
			surface_destroy(local_surface);
			return false;
		}
		
		sig_connect(&canvas->keyboard_event, NULL, on_keyboard_event);
	}
	
	if (surface != NULL)
		surface_destroy(surface);
	
	surface = local_surface;
	
	return true;
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Compositor server not specified.\n");
		return 1;
	}
	
	if (argc < 3) {
		printf("No image files specified.\n");
		return 1;
	}
	
	main_window = window_open(argv[1], true, false, "viewer");
	if (!main_window) {
		printf("Cannot open main window.\n");
		return 2;
	}
	
	imgs_count = argc - 2;
	imgs = calloc(imgs_count, sizeof(char *));
	if (imgs == NULL) {
		printf("Out of memory.\n");
		return 3;
	}
	
	for (int i = 0; i < argc - 2; i++) {
		imgs[i] = str_dup(argv[i + 2]);
		if (imgs[i] == NULL) {
			printf("Out of memory.\n");
			return 4;
		}
	}
	
	if (!img_load(imgs[imgs_current])) {
		printf("Cannot load image \"%s\".\n", imgs[imgs_current]);
		return 2;
	}
	
	window_resize(main_window, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
	    WINDOW_PLACEMENT_ABSOLUTE);
	window_exec(main_window);
	
	task_retval(0);
	async_manager();
	
	return 0;
}

/** @}
 */
