/*
 * Copyright (c) 2024 Jiri Svoboda
 * Copyright (c) 2014 Martin Decky
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

/** @addtogroup barber
 * @{
 */
/** @file
 */

#include <device/led_dev.h>
#include <errno.h>
#include <fibril_synch.h>
#include <gfximage/tga_gz.h>
#include <io/pixel.h>
#include <loc.h>
#include <stats.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <ui/ui.h>
#include <ui/wdecor.h>
#include <ui/window.h>
#include <ui/image.h>
#include "images.h"
#include "images_tiny.h"

#define NAME  "barber"

#define FRAMES  IMAGES

#define MIN_FPS  1
#define MAX_FPS  25

#define MIN_LOAD  (LOAD_UNIT / 4)
#define MAX_LOAD  (LOAD_UNIT / 3)

#define LED_PERIOD  1000000
#define LED_COLORS  7

typedef struct {
	link_t link;
	service_id_t svc_id;
	async_sess_t *sess;
} led_dev_t;

typedef struct {
	ui_t *ui;
} barber_t;

static fibril_timer_t *led_timer = NULL;
static list_t led_devs;
static unsigned int led_color = 0;

static pixel_t led_colors[LED_COLORS] = {
	PIXEL(0xff, 0xff, 0x00, 0x00),
	PIXEL(0xff, 0x00, 0xff, 0x00),
	PIXEL(0xff, 0x00, 0x00, 0xff),
	PIXEL(0xff, 0xff, 0xff, 0x00),
	PIXEL(0xff, 0xff, 0x00, 0xff),
	PIXEL(0xff, 0x00, 0xff, 0xff),
	PIXEL(0xff, 0xff, 0xff, 0xff)
};

static fibril_timer_t *frame_timer = NULL;
static ui_image_t *frame_img;
static gfx_bitmap_t *frame_bmp[FRAMES];

static unsigned int frame = 0;
static unsigned int fps = MIN_FPS;
static gfx_coord_t frame_width;
static gfx_coord_t frame_height;

static void led_timer_callback(void *);
static void frame_timer_callback(void *);

static void wnd_close(ui_window_t *, void *);
static void wnd_kbd_event(ui_window_t *, void *, kbd_event_t *);

static ui_window_cb_t window_cb = {
	.close = wnd_close,
	.kbd = wnd_kbd_event
};

/** Window close button was clicked.
 *
 * @param window Window
 * @param arg Argument (barber)
 */
static void wnd_close(ui_window_t *window, void *arg)
{
	barber_t *barber = (barber_t *) arg;

	ui_quit(barber->ui);
}

/** Barber unmodified key press.
 *
 * @param barber Barber
 * @param event Keyboard event
 */
static void barber_kbd_event_unmod(barber_t *barber, kbd_event_t *event)
{
	if (event->key == KC_ESCAPE)
		ui_quit(barber->ui);
}

/** Barber ctrl-key key press.
 *
 * @param barber Barber
 * @param event Keyboard event
 */
static void barber_kbd_event_ctrl(barber_t *barber, kbd_event_t *event)
{
	if (event->key == KC_Q)
		ui_quit(barber->ui);
}

/** Barber window keyboard event.
 *
 * @param window UI window
 * @param arg Argument (barber_t *)
 * @param event Keyboard event
 */
static void wnd_kbd_event(ui_window_t *window, void *arg, kbd_event_t *event)
{
	barber_t *barber = (barber_t *)arg;

	if (event->type != KEY_PRESS)
		return;

	if ((event->mods & (KM_CTRL | KM_ALT | KM_SHIFT)) == 0)
		barber_kbd_event_unmod(barber, event);

	if ((event->mods & KM_CTRL) != 0 &&
	    (event->mods & (KM_ALT | KM_SHIFT)) == 0)
		barber_kbd_event_ctrl(barber, event);

	ui_window_def_kbd(window, event);
}

static bool decode_frames(gfx_context_t *gc, image_t *img)
{
	gfx_rect_t rect;
	errno_t rc;

	for (unsigned int i = 0; i < FRAMES; i++) {
		rc = decode_tga_gz(gc, img[i].addr, img[i].size,
		    &frame_bmp[i], &rect);
		if (rc != EOK) {
			printf("Unable to decode frame %u.\n", i);
			return false;
		}

		(void)rect;
	}

	return true;
}

static void destroy_frames(void)
{
	unsigned i;

	for (i = 0; i < FRAMES; i++) {
		gfx_bitmap_destroy(frame_bmp[i]);
		frame_bmp[i] = NULL;
	}
}

static void plan_led_timer(void)
{
	fibril_timer_set(led_timer, LED_PERIOD, led_timer_callback, NULL);
}

static load_t get_load(void)
{
	size_t count;
	load_t *load = stats_get_load(&count);
	load_t load_val;

	if ((load != NULL) && (count > 0)) {
		load_val = load[0];
		free(load);
	} else
		load_val = 0;

	return load_val;
}

static void plan_frame_timer(usec_t render_time)
{
	/*
	 * Crank up the FPS unless we lack
	 * behind with the rendering and
	 * unless the load is not above
	 * a lower threshold.
	 */

	usec_t delta = 1000000 / fps;
	load_t load = get_load();

	if ((delta >= render_time) && (load < MIN_LOAD))
		fps++;

	if (fps > MAX_FPS)
		fps = MAX_FPS;

	/*
	 * If we lack behind then immediately
	 * go to the lowest FPS.
	 */

	if (delta < render_time)
		fps = MIN_FPS;

	/*
	 * Crank down the FPS if the current
	 * load is above an upper threshold.
	 */

	if (load > MAX_LOAD)
		fps--;

	if (fps < MIN_FPS)
		fps = MIN_FPS;

	delta = 1000000 / fps;

	fibril_timer_set(frame_timer, delta, frame_timer_callback, NULL);
}

static void led_timer_callback(void *data)
{
	pixel_t next_led_color = led_colors[led_color];

	led_color++;
	if (led_color >= LED_COLORS)
		led_color = 0;

	list_foreach(led_devs, link, led_dev_t, dev) {
		if (dev->sess)
			led_dev_color_set(dev->sess, next_led_color);
	}

	plan_led_timer();
}

static void frame_timer_callback(void *data)
{
	struct timespec prev;
	gfx_rect_t rect;
	getuptime(&prev);

	frame++;
	if (frame >= FRAMES)
		frame = 0;

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = frame_width;
	rect.p1.y = frame_height;

	ui_image_set_bmp(frame_img, frame_bmp[frame], &rect);
	(void) ui_image_paint(frame_img);

	struct timespec cur;
	getuptime(&cur);

	plan_frame_timer(NSEC2USEC(ts_sub_diff(&cur, &prev)));
}

static void loc_callback(void *arg)
{
	category_id_t led_cat;
	errno_t rc = loc_category_get_id("led", &led_cat, IPC_FLAG_BLOCKING);
	if (rc != EOK)
		return;

	service_id_t *svcs;
	size_t count;
	rc = loc_category_get_svcs(led_cat, &svcs, &count);
	if (rc != EOK)
		return;

	for (size_t i = 0; i < count; i++) {
		bool known = false;

		/* Determine whether we already know this device. */
		list_foreach(led_devs, link, led_dev_t, dev) {
			if (dev->svc_id == svcs[i]) {
				known = true;
				break;
			}
		}

		if (!known) {
			led_dev_t *dev = (led_dev_t *) calloc(1, sizeof(led_dev_t));
			if (!dev)
				continue;

			link_initialize(&dev->link);
			dev->svc_id = svcs[i];
			dev->sess = loc_service_connect(svcs[i], INTERFACE_DDF, 0);

			list_append(&dev->link, &led_devs);
		}
	}

	// FIXME: Handle LED device removal

	free(svcs);
}

static void print_syntax(void)
{
	printf("Syntax: %s [-d <display>]\n", NAME);
}

int main(int argc, char *argv[])
{
	const char *display_spec = UI_ANY_DEFAULT;
	barber_t barber;
	ui_t *ui;
	ui_wnd_params_t params;
	ui_window_t *window;
	ui_resource_t *ui_res;
	gfx_rect_t rect;
	gfx_rect_t wrect;
	gfx_rect_t app_rect;
	gfx_context_t *gc;
	gfx_coord2_t off;
	image_t *img;
	int i;

	i = 1;
	while (i < argc) {
		if (str_cmp(argv[i], "-d") == 0) {
			++i;
			if (i >= argc) {
				printf("Argument missing.\n");
				print_syntax();
				return 1;
			}

			display_spec = argv[i++];
		} else {
			printf("Invalid option '%s'.\n", argv[i]);
			print_syntax();
			return 1;
		}
	}

	list_initialize(&led_devs);
	errno_t rc = loc_register_cat_change_cb(loc_callback, NULL);
	if (rc != EOK) {
		printf("Unable to register callback for device discovery.\n");
		return 1;
	}

	led_timer = fibril_timer_create(NULL);
	if (!led_timer) {
		printf("Unable to create LED timer.\n");
		return 1;
	}

	frame_timer = fibril_timer_create(NULL);
	if (!frame_timer) {
		printf("Unable to create frame timer.\n");
		return 1;
	}

	rc = ui_create(display_spec, &ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		return 1;
	}

	if (ui_is_textmode(ui)) {
		frame_width = 10;
		frame_height = 16;
	} else {
		frame_width = 59;
		frame_height = 192;
	}

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = frame_width;
	rect.p1.y = frame_height;

	ui_wnd_params_init(&params);
	params.caption = "Barber Pole";
	params.placement = ui_wnd_place_bottom_right;
	/*
	 * Compute window rectangle such that application area corresponds
	 * to rect
	 */
	ui_wdecor_rect_from_app(ui, params.style, &rect, &wrect);
	off = wrect.p0;
	gfx_rect_rtranslate(&off, &wrect, &params.rect);

	barber.ui = ui;

	rc = ui_window_create(ui, &params, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		return 1;
	}

	ui_res = ui_window_get_res(window);
	gc = ui_window_get_gc(window);
	ui_window_get_app_rect(window, &app_rect);
	ui_window_set_cb(window, &window_cb, (void *) &barber);

	img = ui_is_textmode(ui) ? image_tinys : images;

	if (!decode_frames(gc, img))
		return 1;

	rc = ui_image_create(ui_res, frame_bmp[frame], &rect,
	    &frame_img);
	if (rc != EOK) {
		printf("Error creating UI.\n");
		return 1;
	}

	ui_image_set_rect(frame_img, &app_rect);

	ui_window_add(window, ui_image_ctl(frame_img));

	rc = ui_window_paint(window);
	if (rc != EOK) {
		printf("Error painting window.\n");
		return 1;
	}

	plan_led_timer();
	plan_frame_timer(0);

	ui_run(ui);

	/* Unlink bitmap from image so it is not destroyed along with it */
	ui_image_set_bmp(frame_img, NULL, &rect);

	destroy_frames();

	ui_window_destroy(window);
	ui_destroy(ui);

	return 0;
}

/** @}
 */
