/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup display
 * @{
 */
/**
 * @file Display server input event types
 */

#ifndef TYPES_DISPLAY_IEVENT_H
#define TYPES_DISPLAY_IEVENT_H

#include <adt/list.h>
#include <io/kbd_event.h>
#include "ptd_event.h"

/** Display server input event type */
typedef enum {
	/** Keyboard event */
	det_kbd,
	/** Pointing device event */
	det_ptd
} ds_ievent_type_t;

/** Display server input event */
typedef struct ds_ievent {
	/** Parent display */
	struct ds_display *display;
	/** Link to display->ievents */
	link_t lievents;
	/** Input event type */
	ds_ievent_type_t etype;
	/** Event data */
	union {
		/** Keyboard event */
		kbd_event_t kbd;
		/** Pointing device ievent */
		ptd_event_t ptd;
	} ev;
} ds_ievent_t;

#endif

/** @}
 */
