/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup output
 * @{
 */

#ifndef OUTPUT_PROTO_VT100_H_
#define OUTPUT_PROTO_VT100_H_

#include <io/charfield.h>

typedef void (*vt100_putuchar_t)(char32_t ch);
typedef void (*vt100_control_puts_t)(const char *str);
typedef void (*vt100_flush_t)(void);

typedef struct {
	sysarg_t cols;
	sysarg_t rows;

	sysarg_t cur_col;
	sysarg_t cur_row;
	char_attrs_t cur_attrs;

	vt100_putuchar_t putuchar;
	vt100_control_puts_t control_puts;
	vt100_flush_t flush;
} vt100_state_t;

extern vt100_state_t *vt100_state_create(sysarg_t, sysarg_t, vt100_putuchar_t,
    vt100_control_puts_t, vt100_flush_t);
extern void vt100_state_destroy(vt100_state_t *);

extern errno_t vt100_yield(vt100_state_t *);
extern errno_t vt100_claim(vt100_state_t *);
extern void vt100_get_dimensions(vt100_state_t *, sysarg_t *, sysarg_t *);

extern void vt100_goto(vt100_state_t *, sysarg_t, sysarg_t);
extern void vt100_set_attr(vt100_state_t *, char_attrs_t);
extern void vt100_cursor_visibility(vt100_state_t *, bool);
extern void vt100_putuchar(vt100_state_t *, char32_t);
extern void vt100_flush(vt100_state_t *);

#endif

/** @}
 */
