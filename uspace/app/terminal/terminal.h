/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 * SPDX-FileCopyrightText: 2012 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup terminal
 * @{
 */
/**
 * @file
 */

#ifndef TERMINAL_H
#define TERMINAL_H

#include <adt/prodcons.h>
#include <errno.h>
#include <fibril.h>
#include <fibril_synch.h>
#include <gfx/bitmap.h>
#include <gfx/context.h>
#include <gfx/coord.h>
#include <io/chargrid.h>
#include <io/con_srv.h>
#include <loc.h>
#include <stdatomic.h>
#include <str.h>
#include <task.h>
#include <ui/ui.h>
#include <ui/window.h>

#define UTF8_CHAR_BUFFER_SIZE  (STR_BOUNDS(1) + 1)

typedef enum {
	tf_topleft = 1
} terminal_flags_t;

typedef struct {
	ui_t *ui;
	ui_window_t *window;
	ui_resource_t *ui_res;
	gfx_context_t *gc;

	gfx_bitmap_t *bmp;
	sysarg_t w;
	sysarg_t h;
	gfx_rect_t update;
	gfx_coord2_t off;
	bool is_focused;

	fibril_mutex_t mtx;
	link_t link;
	atomic_flag refcnt;

	prodcons_t input_pc;
	char char_remains[UTF8_CHAR_BUFFER_SIZE];
	size_t char_remains_len;

	sysarg_t cols;
	sysarg_t rows;
	chargrid_t *frontbuf;
	chargrid_t *backbuf;
	sysarg_t top_row;

	sysarg_t ucols;
	sysarg_t urows;
	charfield_t *ubuf;

	service_id_t dsid;
	con_srvs_t srvs;

	task_wait_t wait;
	fid_t wfid;
} terminal_t;

extern errno_t terminal_create(const char *, sysarg_t, sysarg_t,
    terminal_flags_t, const char *, terminal_t **);
extern void terminal_destroy(terminal_t *);

#endif

/** @}
 */
