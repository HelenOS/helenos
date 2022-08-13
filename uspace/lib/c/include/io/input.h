/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_IO_INPUT_H_
#define _LIBC_IO_INPUT_H_

#include <async.h>
#include <io/kbd_event.h>

struct input_ev_ops;

typedef struct {
	async_sess_t *sess;
	struct input_ev_ops *ev_ops;
	void *user;
} input_t;

typedef struct input_ev_ops {
	errno_t (*active)(input_t *);
	errno_t (*deactive)(input_t *);
	errno_t (*key)(input_t *, kbd_event_type_t, keycode_t, keymod_t, char32_t);
	errno_t (*move)(input_t *, int, int);
	errno_t (*abs_move)(input_t *, unsigned, unsigned, unsigned, unsigned);
	errno_t (*button)(input_t *, int, int);
	errno_t (*dclick)(input_t *, int);
} input_ev_ops_t;

extern errno_t input_open(async_sess_t *, input_ev_ops_t *, void *, input_t **);
extern void input_close(input_t *);
extern errno_t input_activate(input_t *);

#endif

/** @}
 */
