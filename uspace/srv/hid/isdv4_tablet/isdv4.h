/*
 * Copyright (c) 2012 Martin Sucha
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

#ifndef ISDV4_H_
#define ISDV4_H_

#include <async.h>
#include <io/chardev.h>

typedef struct isdv4_event isdv4_event_t;

typedef void (*isdv4_event_fn)(const isdv4_event_t *);

typedef struct {
	/* Stylus information */
	unsigned int stylus_max_x;
	unsigned int stylus_max_y;
	unsigned int stylus_max_pressure;
	unsigned int stylus_max_xtilt;
	unsigned int stylus_max_ytilt;
	bool stylus_tilt_supported;

	/* Touch information */
	unsigned int touch_type;
	unsigned int touch_max_x;
	unsigned int touch_max_y;

	/* Event state */
	bool stylus_in_proximity;
	bool stylus_is_eraser;
	bool tip_pressed; /* Reported as stylus button 1 */
	bool button1_pressed; /* Reported as stylus button 2 */
	bool button2_pressed; /* Reported as stylus button 3 */
	bool finger1_pressed; /* Reported as touch button 1 */

	/** Session with the serial device */
	async_sess_t *sess;
	/** Character device */
	chardev_t *chardev;

	/* Receive buffer state */
	uint8_t *buf;
	size_t buf_size;
	size_t buf_end;

	/** Callbacks */
	isdv4_event_fn emit_event_fn;
} isdv4_state_t;

typedef enum {
	UNKNOWN,
	PRESS,
	RELEASE,
	PROXIMITY_IN,
	PROXIMITY_OUT,
	MOVE
} isdv4_event_type_t;

typedef enum {
	STYLUS_TIP,
	STYLUS_ERASER,
	TOUCH
} isdv4_source_type_t;

struct isdv4_event {
	isdv4_event_type_t type;
	isdv4_source_type_t source;
	unsigned int x;
	unsigned int y;
	unsigned int pressure;
	unsigned int button;
};

extern errno_t isdv4_init(isdv4_state_t *, async_sess_t *, isdv4_event_fn);
extern errno_t isdv4_init_tablet(isdv4_state_t *);
extern errno_t isdv4_read_events(isdv4_state_t *state);
extern void isdv4_fini(isdv4_state_t *);

#endif
