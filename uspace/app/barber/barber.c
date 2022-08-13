/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 * SPDX-FileCopyrightText: 2014 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

#define NAME  "barber"

#define FRAMES  IMAGES

#define MIN_FPS  1
#define MAX_FPS  25

#define MIN_LOAD  (LOAD_UNIT / 4)
#define MAX_LOAD  (LOAD_UNIT / 3)

#define FRAME_WIDTH   59
#define FRAME_HEIGHT  192

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

static void led_timer_callback(void *);
static void frame_timer_callback(void *);

static void wnd_close(ui_window_t *, void *);

static ui_window_cb_t window_cb = {
	.close = wnd_close
};

/** Window close button was clicked.
 *
 * @param window Window
 * @param arg Argument (launcher)
 */
static void wnd_close(ui_window_t *window, void *arg)
{
	barber_t *barber = (barber_t *) arg;

	ui_quit(barber->ui);
}

static bool decode_frames(gfx_context_t *gc)
{
	gfx_rect_t rect;
	errno_t rc;

	for (unsigned int i = 0; i < FRAMES; i++) {
		rc = decode_tga_gz(gc, images[i].addr, images[i].size,
		    &frame_bmp[i], &rect);
		if (rc != EOK) {
			printf("Unable to decode frame %u.\n", i);
			return false;
		}

		(void) rect;
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
	rect.p1.x = FRAME_WIDTH;
	rect.p1.y = FRAME_HEIGHT;

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
	const char *display_spec = UI_DISPLAY_DEFAULT;
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

	rect.p0.x = 0;
	rect.p0.y = 0;
	rect.p1.x = FRAME_WIDTH;
	rect.p1.y = FRAME_HEIGHT;

	ui_wnd_params_init(&params);
	params.caption = "";
	params.placement = ui_wnd_place_bottom_right;
	/*
	 * Compute window rectangle such that application area corresponds
	 * to rect
	 */
	ui_wdecor_rect_from_app(params.style, &rect, &wrect);
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

	if (!decode_frames(gc))
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
