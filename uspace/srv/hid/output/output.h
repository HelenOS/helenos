/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup output
 * @{
 */

#ifndef OUTPUT_OUTPUT_H_
#define OUTPUT_OUTPUT_H_

#include <adt/list.h>
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
