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

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <macros.h>
#include <as.h>
#include <task.h>
#include <ipc/output.h>
#include <config.h>
#include "port/ega.h"
#include "port/chardev.h"
#include "output.h"

#define MAX_COLS  128
#define MAX_ROWS  128

typedef struct {
	link_t link;

	size_t size;
	unsigned int flags;
	void *data;
} frontbuf_t;

static LIST_INITIALIZE(outdevs);
static LIST_INITIALIZE(frontbufs);

outdev_t *outdev_register(outdev_ops_t *ops, void *data)
{
	assert(ops->get_dimensions);

	outdev_t *dev = (outdev_t *) malloc(sizeof(outdev_t));
	if (dev == NULL)
		return NULL;

	link_initialize(&dev->link);

	dev->ops = *ops;
	dev->data = data;

	ops->get_dimensions(dev, &dev->cols, &dev->rows);
	dev->backbuf = chargrid_create(dev->cols, dev->rows,
	    CHARGRID_FLAG_NONE);
	if (dev->backbuf == NULL) {
		free(dev);
		return NULL;
	}

	list_append(&dev->link, &outdevs);
	return dev;
}

static void srv_yield(ipc_callid_t iid, ipc_call_t *icall)
{
	errno_t ret = EOK;

	list_foreach(outdevs, link, outdev_t, dev) {
		assert(dev->ops.yield);

		errno_t rc = dev->ops.yield(dev);
		if (rc != EOK)
			ret = rc;
	}

	async_answer_0(iid, ret);
}

static void srv_claim(ipc_callid_t iid, ipc_call_t *icall)
{
	errno_t ret = EOK;

	list_foreach(outdevs, link, outdev_t, dev) {
		assert(dev->ops.claim);

		errno_t rc = dev->ops.claim(dev);
		if (rc != EOK)
			ret = rc;
	}

	async_answer_0(iid, ret);
}

static void srv_get_dimensions(ipc_callid_t iid, ipc_call_t *icall)
{
	sysarg_t cols = MAX_COLS;
	sysarg_t rows = MAX_ROWS;

	list_foreach(outdevs, link, outdev_t, dev) {
		cols = min(cols, dev->cols);
		rows = min(rows, dev->rows);
	}

	async_answer_2(iid, EOK, cols, rows);
}

static void srv_get_caps(ipc_callid_t iid, ipc_call_t *icall)
{
	console_caps_t caps = 0;

	list_foreach(outdevs, link, outdev_t, dev) {
		assert(dev->ops.get_caps);

		caps |= dev->ops.get_caps(dev);
	}

	async_answer_1(iid, EOK, caps);
}

static frontbuf_t *resolve_frontbuf(sysarg_t handle, ipc_callid_t iid)
{
	frontbuf_t *frontbuf = NULL;
	list_foreach(frontbufs, link, frontbuf_t, cur) {
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

static void srv_frontbuf_create(ipc_callid_t iid, ipc_call_t *icall)
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

	errno_t rc = async_share_out_finalize(callid, &frontbuf->data);
	if ((rc != EOK) || (frontbuf->data == AS_MAP_FAILED)) {
		free(frontbuf);
		async_answer_0(iid, ENOMEM);
		return;
	}

	list_append(&frontbuf->link, &frontbufs);
	async_answer_1(iid, EOK, (sysarg_t) frontbuf);
}

static void srv_frontbuf_destroy(ipc_callid_t iid, ipc_call_t *icall)
{
	frontbuf_t *frontbuf = resolve_frontbuf(IPC_GET_ARG1(*icall), iid);
	if (frontbuf == NULL)
		return;

	list_remove(&frontbuf->link);
	as_area_destroy(frontbuf->data);
	free(frontbuf);

	async_answer_0(iid, EOK);
}

static void srv_cursor_update(ipc_callid_t iid, ipc_call_t *icall)
{
	frontbuf_t *frontbuf = resolve_frontbuf(IPC_GET_ARG1(*icall), iid);
	if (frontbuf == NULL)
		return;

	chargrid_t *buf = (chargrid_t *) frontbuf->data;
	bool visible = chargrid_get_cursor_visibility(buf);

	sysarg_t col;
	sysarg_t row;
	chargrid_get_cursor(buf, &col, &row);

	list_foreach(outdevs, link, outdev_t, dev) {
		assert(dev->ops.cursor_update);

		sysarg_t prev_col;
		sysarg_t prev_row;
		chargrid_get_cursor(dev->backbuf, &prev_col, &prev_row);

		chargrid_set_cursor(dev->backbuf, col, row);
		chargrid_set_cursor_visibility(dev->backbuf, visible);

		dev->ops.cursor_update(dev, prev_col, prev_row, col, row,
		    visible);
		dev->ops.flush(dev);

	}

	async_answer_0(iid, EOK);
}

static void srv_set_style(ipc_callid_t iid, ipc_call_t *icall)
{
	list_foreach(outdevs, link, outdev_t, dev) {
		dev->attrs.type = CHAR_ATTR_STYLE;
		dev->attrs.val.style =
		    (console_style_t) IPC_GET_ARG1(*icall);
	}

	async_answer_0(iid, EOK);
}

static void srv_set_color(ipc_callid_t iid, ipc_call_t *icall)
{
	list_foreach(outdevs, link, outdev_t, dev) {
		dev->attrs.type = CHAR_ATTR_INDEX;
		dev->attrs.val.index.bgcolor =
		    (console_color_t) IPC_GET_ARG1(*icall);
		dev->attrs.val.index.fgcolor =
		    (console_color_t) IPC_GET_ARG2(*icall);
		dev->attrs.val.index.attr =
		    (console_color_attr_t) IPC_GET_ARG3(*icall);
	}

	async_answer_0(iid, EOK);
}

static void srv_set_rgb_color(ipc_callid_t iid, ipc_call_t *icall)
{
	list_foreach(outdevs, link, outdev_t, dev) {
		dev->attrs.type = CHAR_ATTR_RGB;
		dev->attrs.val.rgb.bgcolor = IPC_GET_ARG1(*icall);
		dev->attrs.val.rgb.fgcolor = IPC_GET_ARG2(*icall);
	}

	async_answer_0(iid, EOK);
}

static bool srv_update_scroll(outdev_t *dev, chargrid_t *buf)
{
	assert(dev->ops.char_update);

	sysarg_t top_row = chargrid_get_top_row(buf);

	if (dev->top_row == top_row)
		return false;

	dev->top_row = top_row;

	for (sysarg_t y = 0; y < dev->rows; y++) {
		for (sysarg_t x = 0; x < dev->cols; x++) {
			charfield_t *front_field =
			    chargrid_charfield_at(buf, x, y);
			charfield_t *back_field =
			    chargrid_charfield_at(dev->backbuf, x, y);
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
				dev->ops.char_update(dev, x, y);
		}
	}

	return true;
}

static void srv_update(ipc_callid_t iid, ipc_call_t *icall)
{
	frontbuf_t *frontbuf = resolve_frontbuf(IPC_GET_ARG1(*icall), iid);
	if (frontbuf == NULL)
		return;

	chargrid_t *buf = (chargrid_t *) frontbuf->data;

	list_foreach(outdevs, link, outdev_t, dev) {
		assert(dev->ops.char_update);

		if (srv_update_scroll(dev, buf))
			continue;

		for (sysarg_t y = 0; y < dev->rows; y++) {
			for (sysarg_t x = 0; x < dev->cols; x++) {
				charfield_t *front_field =
				    chargrid_charfield_at(buf, x, y);
				charfield_t *back_field =
				    chargrid_charfield_at(dev->backbuf, x, y);
				bool update = false;

				if ((front_field->flags & CHAR_FLAG_DIRTY) ==
				    CHAR_FLAG_DIRTY) {
					if (front_field->ch != back_field->ch) {
						back_field->ch = front_field->ch;
						update = true;
					}

					if (!attrs_same(front_field->attrs,
					    back_field->attrs)) {
						back_field->attrs = front_field->attrs;
						update = true;
					}

					front_field->flags &= ~CHAR_FLAG_DIRTY;
				}

				if (update)
					dev->ops.char_update(dev, x, y);
			}
		}

		dev->ops.flush(dev);
	}


	async_answer_0(iid, EOK);
}

static void srv_damage(ipc_callid_t iid, ipc_call_t *icall)
{
	frontbuf_t *frontbuf = resolve_frontbuf(IPC_GET_ARG1(*icall), iid);
	if (frontbuf == NULL)
		return;

	chargrid_t *buf = (chargrid_t *) frontbuf->data;

	list_foreach(outdevs, link, outdev_t, dev) {
		assert(dev->ops.char_update);

		if (srv_update_scroll(dev, buf))
			continue;

		sysarg_t col = IPC_GET_ARG2(*icall);
		sysarg_t row = IPC_GET_ARG3(*icall);

		sysarg_t cols = IPC_GET_ARG4(*icall);
		sysarg_t rows = IPC_GET_ARG5(*icall);

		for (sysarg_t y = 0; y < rows; y++) {
			for (sysarg_t x = 0; x < cols; x++) {
				charfield_t *front_field =
				    chargrid_charfield_at(buf, col + x, row + y);
				charfield_t *back_field =
				    chargrid_charfield_at(dev->backbuf, col + x, row + y);

				back_field->ch = front_field->ch;
				back_field->attrs = front_field->attrs;
				front_field->flags &= ~CHAR_FLAG_DIRTY;
				dev->ops.char_update(dev, col + x, row + y);
			}
		}
		dev->ops.flush(dev);

	}
	async_answer_0(iid, EOK);
}

static void client_connection(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	/* Accept the connection */
	async_answer_0(iid, EOK);

	while (true) {
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);

		if (!IPC_GET_IMETHOD(call)) {
			async_answer_0(callid, EOK);
			break;
		}

		switch (IPC_GET_IMETHOD(call)) {
		case OUTPUT_YIELD:
			srv_yield(callid, &call);
			break;
		case OUTPUT_CLAIM:
			srv_claim(callid, &call);
			break;
		case OUTPUT_GET_DIMENSIONS:
			srv_get_dimensions(callid, &call);
			break;
		case OUTPUT_GET_CAPS:
			srv_get_caps(callid, &call);
			break;

		case OUTPUT_FRONTBUF_CREATE:
			srv_frontbuf_create(callid, &call);
			break;
		case OUTPUT_FRONTBUF_DESTROY:
			srv_frontbuf_destroy(callid, &call);
			break;

		case OUTPUT_CURSOR_UPDATE:
			srv_cursor_update(callid, &call);
			break;
		case OUTPUT_SET_STYLE:
			srv_set_style(callid, &call);
			break;
		case OUTPUT_SET_COLOR:
			srv_set_color(callid, &call);
			break;
		case OUTPUT_SET_RGB_COLOR:
			srv_set_rgb_color(callid, &call);
			break;
		case OUTPUT_UPDATE:
			srv_update(callid, &call);
			break;
		case OUTPUT_DAMAGE:
			srv_damage(callid, &call);
			break;

		default:
			async_answer_0(callid, EINVAL);
		}
	}
}

static void usage(char *name)
{
	printf("Usage: %s <service_name>\n", name);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		usage(argv[0]);
		return 1;
	}

	printf("%s: HelenOS output service\n", NAME);

	/* Register server */
	async_set_fallback_port_handler(client_connection, NULL);
	errno_t rc = loc_server_register(NAME);
	if (rc != EOK) {
		printf("%s: Unable to register driver\n", NAME);
		return rc;
	}

	service_id_t service_id;
	rc = loc_service_register(argv[1], &service_id);
	if (rc != EOK) {
		printf("%s: Unable to register service %s\n", NAME, argv[1]);
		return rc;
	}

	if (!config_key_exists("console")) {
		ega_init();
	}

	chardev_init();

	printf("%s: Accepting connections\n", NAME);
	task_retval(0);
	async_manager();

	/* Never reached */
	return 0;
}
