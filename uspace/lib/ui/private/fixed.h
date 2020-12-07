/*
 * Copyright (c) 2020 Jiri Svoboda
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

/** @addtogroup libui
 * @{
 */
/**
 * @file Fixed layout structure
 *
 */

#ifndef _UI_PRIVATE_FIXED_H
#define _UI_PRIVATE_FIXED_H

#include <adt/list.h>
#include <gfx/coord.h>
#include <stdbool.h>
#include <types/ui/fixed.h>

/** Actual structure of fixed layout.
 *
 * This is private to libui.
 */
struct ui_fixed {
	/** Base control object */
	struct ui_control *control;
	/** Layout elements (ui_fixed_elem_t) */
	list_t elem;
};

/** Fixed layout element. */
typedef struct {
	/** Containing fixed layout */
	struct ui_fixed *fixed;
	/** Link to @c fixed->elem list */
	link_t lelems;
	/** Control */
	ui_control_t *control;
} ui_fixed_elem_t;

extern ui_fixed_elem_t *ui_fixed_first(ui_fixed_t *f);
extern ui_fixed_elem_t *ui_fixed_next(ui_fixed_elem_t *);

#endif

/** @}
 */
