/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup output
 * @{
 */

#ifndef OUTPUT_OUTPUT_H_
#define OUTPUT_OUTPUT_H_

#include <stdbool.h>
#include <loc.h>
#include <io/console.h>
#include <io/style.h>
#include <io/color.h>
#include <io/chargrid.h>

#define NAME  "output"

struct outdev;

typedef struct {
	errno_t (*yield)(struct outdev *dev);
	errno_t (*claim)(struct outdev *dev);

	void (*get_dimensions)(struct outdev *dev, sysarg_t *cols,
	    sysarg_t *rows);
	console_caps_t (*get_caps)(struct outdev *dev);

	void (*cursor_update)(struct outdev *dev, sysarg_t prev_col,
	    sysarg_t prev_row, sysarg_t col, sysarg_t row, bool visible);
	void (*char_update)(struct outdev *dev, sysarg_t col, sysarg_t row);
	void (*flush)(struct outdev *dev);
} outdev_ops_t;

typedef struct outdev {
	link_t link;

	sysarg_t cols;
	sysarg_t rows;
	char_attrs_t attrs;

	chargrid_t *backbuf;
	sysarg_t top_row;

	outdev_ops_t ops;
	void *data;
} outdev_t;

extern outdev_t *outdev_register(outdev_ops_t *, void *);

#endif

/** @}
 */
