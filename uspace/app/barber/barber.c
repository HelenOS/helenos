/*
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

#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <malloc.h>
#include <task.h>
#include <loc.h>
#include <stats.h>
#include <fibril_synch.h>
#include <io/pixel.h>
#include <device/led_dev.h>
#include <window.h>
#include <canvas.h>
#include <surface.h>
#include <codec/tga.gz.h>
#include "images.h"

#define NAME  "barber"

#define FRAMES  30

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

static char *winreg = NULL;

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
static canvas_t *frame_canvas;
static surface_t *frames[FRAMES];

static unsigned int frame = 0;
static unsigned int fps = MIN_FPS;

static void led_timer_callback(void *);
static void frame_timer_callback(void *);

static bool decode_frames(void)
{
	frames[0] = decode_tga_gz((void *) frame01_tga_gz, frame01_tga_gz_size, 0);
	frames[1] = decode_tga_gz((void *) frame02_tga_gz, frame02_tga_gz_size, 0);
	frames[2] = decode_tga_gz((void *) frame03_tga_gz, frame03_tga_gz_size, 0);
	frames[3] = decode_tga_gz((void *) frame04_tga_gz, frame04_tga_gz_size, 0);
	frames[4] = decode_tga_gz((void *) frame05_tga_gz, frame05_tga_gz_size, 0);
	frames[5] = decode_tga_gz((void *) frame06_tga_gz, frame06_tga_gz_size, 0);
	frames[6] = decode_tga_gz((void *) frame07_tga_gz, frame07_tga_gz_size, 0);
	frames[7] = decode_tga_gz((void *) frame08_tga_gz, frame08_tga_gz_size, 0);
	frames[8] = decode_tga_gz((void *) frame09_tga_gz, frame09_tga_gz_size, 0);
	frames[9] = decode_tga_gz((void *) frame10_tga_gz, frame10_tga_gz_size, 0);
	frames[10] = decode_tga_gz((void *) frame11_tga_gz, frame11_tga_gz_size, 0);
	frames[11] = decode_tga_gz((void *) frame12_tga_gz, frame12_tga_gz_size, 0);
	frames[12] = decode_tga_gz((void *) frame13_tga_gz, frame13_tga_gz_size, 0);
	frames[13] = decode_tga_gz((void *) frame14_tga_gz, frame14_tga_gz_size, 0);
	frames[14] = decode_tga_gz((void *) frame15_tga_gz, frame15_tga_gz_size, 0);
	frames[15] = decode_tga_gz((void *) frame16_tga_gz, frame16_tga_gz_size, 0);
	frames[16] = decode_tga_gz((void *) frame17_tga_gz, frame17_tga_gz_size, 0);
	frames[17] = decode_tga_gz((void *) frame18_tga_gz, frame18_tga_gz_size, 0);
	frames[18] = decode_tga_gz((void *) frame19_tga_gz, frame19_tga_gz_size, 0);
	frames[19] = decode_tga_gz((void *) frame20_tga_gz, frame20_tga_gz_size, 0);
	frames[20] = decode_tga_gz((void *) frame21_tga_gz, frame21_tga_gz_size, 0);
	frames[21] = decode_tga_gz((void *) frame22_tga_gz, frame22_tga_gz_size, 0);
	frames[22] = decode_tga_gz((void *) frame23_tga_gz, frame23_tga_gz_size, 0);
	frames[23] = decode_tga_gz((void *) frame24_tga_gz, frame24_tga_gz_size, 0);
	frames[24] = decode_tga_gz((void *) frame25_tga_gz, frame25_tga_gz_size, 0);
	frames[25] = decode_tga_gz((void *) frame26_tga_gz, frame26_tga_gz_size, 0);
	frames[26] = decode_tga_gz((void *) frame27_tga_gz, frame27_tga_gz_size, 0);
	frames[27] = decode_tga_gz((void *) frame28_tga_gz, frame28_tga_gz_size, 0);
	frames[28] = decode_tga_gz((void *) frame29_tga_gz, frame29_tga_gz_size, 0);
	frames[29] = decode_tga_gz((void *) frame30_tga_gz, frame30_tga_gz_size, 0);
	
	for (unsigned int frame = 0; frame < FRAMES; frame++) {
		if (frames[frame] == NULL) {
			printf("Unable to decode frame %u.\n", frame);
			return false;
		}
	}
	
	return true;
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

static void plan_frame_timer(suseconds_t render_time)
{
	/*
	 * Crank up the FPS unless we lack
	 * behind with the rendering and
	 * unless the load is not above
	 * a lower threshold.
	 */
	
	suseconds_t delta = 1000000 / fps;
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
	struct timeval prev;
	getuptime(&prev);
	
	frame++;
	if (frame >= FRAMES)
		frame = 0;
	
	update_canvas(frame_canvas, frames[frame]);
	
	struct timeval cur;
	getuptime(&cur);
	
	plan_frame_timer(tv_sub(&cur, &prev));
}

static void loc_callback(void)
{
	category_id_t led_cat;
	int rc = loc_category_get_id("led", &led_cat, IPC_FLAG_BLOCKING);
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
			dev->sess = loc_service_connect(EXCHANGE_SERIALIZE, svcs[i], 0);
			
			list_append(&dev->link, &led_devs);
		}
	}
	
	// FIXME: Handle LED device removal
	
	free(svcs);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Compositor server not specified.\n");
		return 1;
	}
	
	list_initialize(&led_devs);
	int rc = loc_register_cat_change_cb(loc_callback);
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
	
	if (!decode_frames())
		return 1;
	
	winreg = argv[1];
	window_t *main_window = window_open(argv[1], true, true, "barber");
	if (!main_window) {
		printf("Cannot open main window.\n");
		return 1;
	}
	
	frame_canvas = create_canvas(window_root(main_window),
	    FRAME_WIDTH, FRAME_HEIGHT, frames[frame]);
	
	if (!frame_canvas) {
		window_close(main_window);
		printf("Cannot create widgets.\n");
		return 1;
	}
	
	window_resize(main_window, 0, 0, FRAME_WIDTH + 8, FRAME_HEIGHT + 28,
	    WINDOW_PLACEMENT_RIGHT | WINDOW_PLACEMENT_BOTTOM);
	window_exec(main_window);
	
	plan_led_timer();
	plan_frame_timer(0);
	
	task_retval(0);
	async_manager();
	
	return 0;
}

/** @}
 */
