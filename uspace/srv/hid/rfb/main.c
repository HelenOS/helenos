/*
 * Copyright (c) 2013 Martin Sucha
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

#include <stdlib.h>
#include <errno.h>
#include <loc.h>
#include <stdio.h>
#include <fibril_synch.h>
#include <abi/ipc/methods.h>
#include <inttypes.h>
#include <io/log.h>
#include <str.h>
#include <task.h>

#include <abi/fb/visuals.h>
#include <adt/list.h>
#include <io/mode.h>
#include <io/pixelmap.h>
#include <io/chargrid.h>
#include <graph.h>

#include "rfb.h"

#define NAME "rfb"

static vslmode_list_element_t pixel_mode;
static visualizer_t *vis;
static rfb_t rfb;

static errno_t rfb_claim(visualizer_t *vs)
{
	return EOK;
}

static errno_t rfb_yield(visualizer_t *vs)
{
	return EOK;
}

static errno_t rfb_suspend(visualizer_t *vs)
{
	return EOK;
}

static errno_t rfb_wakeup(visualizer_t *vs)
{
	return EOK;
}

static errno_t rfb_handle_damage_pixels(visualizer_t *vs,
    sysarg_t x0, sysarg_t y0, sysarg_t width, sysarg_t height,
    sysarg_t x_offset, sysarg_t y_offset)
{
	fibril_mutex_lock(&rfb.lock);

	if (x0 + width > rfb.width || y0 + height > rfb.height) {
		fibril_mutex_unlock(&rfb.lock);
		return EINVAL;
	}

	/* TODO update surface_t and use it */
	if (!rfb.damage_valid) {
		rfb.damage_rect.x = x0;
		rfb.damage_rect.y = y0;
		rfb.damage_rect.width = width;
		rfb.damage_rect.height = height;
		rfb.damage_valid = true;
	} else {
		if (x0 < rfb.damage_rect.x) {
			rfb.damage_rect.width += rfb.damage_rect.x - x0;
			rfb.damage_rect.x = x0;
		}
		if (y0 < rfb.damage_rect.y) {
			rfb.damage_rect.height += rfb.damage_rect.y - y0;
			rfb.damage_rect.y = y0;
		}
		sysarg_t x1 = x0 + width;
		sysarg_t dx1 = rfb.damage_rect.x + rfb.damage_rect.width;
		if (x1 > dx1) {
			rfb.damage_rect.width += x1 - dx1;
		}
		sysarg_t y1 = y0 + height;
		sysarg_t dy1 = rfb.damage_rect.y + rfb.damage_rect.height;
		if (y1 > dy1) {
			rfb.damage_rect.height += y1 - dy1;
		}
	}

	pixelmap_t *map = &vs->cells;

	for (sysarg_t y = y0; y < height + y0; ++y) {
		for (sysarg_t x = x0; x < width + x0; ++x) {
			pixel_t pix = pixelmap_get_pixel(map, (x + x_offset) % map->width,
			    (y + y_offset) % map->height);
			pixelmap_put_pixel(&rfb.framebuffer, x, y, pix);
		}
	}

	fibril_mutex_unlock(&rfb.lock);
	return EOK;
}

static errno_t rfb_change_mode(visualizer_t *vs, vslmode_t new_mode)
{
	return EOK;
}

static visualizer_ops_t rfb_ops = {
	.claim = rfb_claim,
	.yield = rfb_yield,
	.change_mode = rfb_change_mode,
	.handle_damage = rfb_handle_damage_pixels,
	.suspend = rfb_suspend,
	.wakeup = rfb_wakeup
};

static void syntax_print(void)
{
	fprintf(stderr, "Usage: %s <name> <width> <height> [port]\n", NAME);
}

static void client_connection(cap_call_handle_t chandle, ipc_call_t *call, void *data)
{
	graph_visualizer_connection(vis, chandle, call, data);
}

int main(int argc, char **argv)
{
	log_init(NAME);

	if (argc <= 3) {
		syntax_print();
		return 1;
	}

	const char *rfb_name = argv[1];

	char *endptr;
	unsigned long width = strtoul(argv[2], &endptr, 0);
	if (*endptr != 0) {
		fprintf(stderr, "Invalid width\n");
		syntax_print();
		return 1;
	}

	unsigned long height = strtoul(argv[3], &endptr, 0);
	if (*endptr != 0) {
		fprintf(stderr, "Invalid height\n");
		syntax_print();
		return 1;
	}

	unsigned long port = 5900;
	if (argc > 4) {
		port = strtoul(argv[4], &endptr, 0);
		if (*endptr != 0) {
			fprintf(stderr, "Invalid port number\n");
			syntax_print();
			return 1;
		}
	}

	rfb_init(&rfb, width, height, rfb_name);

	vis = malloc(sizeof(visualizer_t));
	if (vis == NULL) {
		fprintf(stderr, "Failed allocating visualizer struct\n");
		return 3;
	}

	graph_init_visualizer(vis);

	pixel_mode.mode.index = 0;
	pixel_mode.mode.version = 0;
	pixel_mode.mode.refresh_rate = 0;
	pixel_mode.mode.screen_aspect.width = rfb.width;
	pixel_mode.mode.screen_aspect.height = rfb.height;
	pixel_mode.mode.screen_width = rfb.width;
	pixel_mode.mode.screen_height = rfb.height;
	pixel_mode.mode.cell_aspect.width = 1;
	pixel_mode.mode.cell_aspect.height = 1;
	pixel_mode.mode.cell_visual.pixel_visual = VISUAL_RGB_8_8_8;

	link_initialize(&pixel_mode.link);
	list_append(&pixel_mode.link, &vis->modes);

	vis->def_mode_idx = 0;

	vis->ops = rfb_ops;
	vis->dev_ctx = NULL;

	async_set_fallback_port_handler(client_connection, NULL);

	errno_t rc = loc_server_register(NAME);
	if (rc != EOK) {
		printf("%s: Unable to register server.\n", NAME);
		return rc;
	}

	char *service_name;
	rc = asprintf(&service_name, "rfb/%s", rfb_name);
	if (rc < 0) {
		printf(NAME ": Unable to create service name\n");
		return rc;
	}

	service_id_t service_id;

	rc = loc_service_register(service_name, &service_id);
	if (rc != EOK) {
		printf(NAME ": Unable to register service %s.\n", service_name);
		return rc;
	}

	free(service_name);

	category_id_t visualizer_category;
	rc = loc_category_get_id("visualizer", &visualizer_category, IPC_FLAG_BLOCKING);
	if (rc != EOK) {
		fprintf(stderr, NAME ": Unable to get visualizer category id.\n");
		return 1;
	}

	rc = loc_service_add_to_cat(service_id, visualizer_category);
	if (rc != EOK) {
		fprintf(stderr, NAME ": Unable to add service to visualizer category.\n");
		return 1;
	}

	rc = rfb_listen(&rfb, port);
	if (rc != EOK) {
		fprintf(stderr, NAME ": Unable to listen at rfb port\n");
		return 2;
	}

	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();

	/* Not reached */
	return 0;
}
