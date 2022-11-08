/*
 * Copyright (c) 2022 Jiri Svoboda
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef TYPES_DISPLAY_PTD_EVENT_H
#define TYPES_DISPLAY_PTD_EVENT_H

#include <gfx/coord.h>

typedef enum {
	PTD_MOVE,
	PTD_ABS_MOVE,
	PTD_PRESS,
	PTD_RELEASE,
	PTD_DCLICK
} ptd_event_type_t;

/** Pointing device event */
typedef struct {
	/** Positioning device ID */
	unsigned pos_id;
	/** PTD event type */
	ptd_event_type_t type;
	/** Button number for PTD_PRESS or PTD_RELEASE */
	int btn_num;
	/** Relative move vector for PTD_MOVE */
	gfx_coord2_t dmove;
	/** Absolute position for PTD_ABS_MOVE */
	gfx_coord2_t apos;
	/** Absolute position bounds for PTD_ABS_MOVE */
	gfx_rect_t abounds;
} ptd_event_t;

#endif

/** @}
 */
