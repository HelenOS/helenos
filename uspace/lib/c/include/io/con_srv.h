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

#ifndef _LIBC_CON_SRV_H_
#define _LIBC_CON_SRV_H_

#include <adt/list.h>
#include <async.h>
#include <fibril_synch.h>
#include <io/charfield.h>
#include <io/color.h>
#include <io/concaps.h>
#include <io/cons_event.h>
#include <io/pixel.h>
#include <io/style.h>
#include <stdbool.h>
#include <time.h>
#include <stddef.h>

typedef struct con_ops con_ops_t;

#define CON_CAPTION_MAXLEN 255

/** Service setup (per sevice) */
typedef struct {
	con_ops_t *ops;
	void *sarg;
	/** Period to check for abort */
	usec_t abort_timeout;
	bool aborted;
} con_srvs_t;

/** Server structure (per client session) */
typedef struct {
	con_srvs_t *srvs;
	async_sess_t *client_sess;
	void *carg;
} con_srv_t;

struct con_ops {
	errno_t (*open)(con_srvs_t *, con_srv_t *);
	errno_t (*close)(con_srv_t *);
	errno_t (*read)(con_srv_t *, void *, size_t, size_t *);
	errno_t (*write)(con_srv_t *, void *, size_t, size_t *);
	void (*sync)(con_srv_t *);
	void (*clear)(con_srv_t *);
	void (*set_pos)(con_srv_t *, sysarg_t col, sysarg_t row);
	errno_t (*get_pos)(con_srv_t *, sysarg_t *, sysarg_t *);
	errno_t (*get_size)(con_srv_t *, sysarg_t *, sysarg_t *);
	errno_t (*get_color_cap)(con_srv_t *, console_caps_t *);
	void (*set_style)(con_srv_t *, console_style_t);
	void (*set_color)(con_srv_t *, console_color_t, console_color_t,
	    console_color_attr_t);
	void (*set_rgb_color)(con_srv_t *, pixel_t, pixel_t);
	void (*set_cursor_visibility)(con_srv_t *, bool);
	errno_t (*set_caption)(con_srv_t *, const char *);
	errno_t (*get_event)(con_srv_t *, cons_event_t *);
	errno_t (*map)(con_srv_t *, sysarg_t, sysarg_t, charfield_t **);
	void (*unmap)(con_srv_t *);
	void (*update)(con_srv_t *, sysarg_t, sysarg_t, sysarg_t, sysarg_t);
};

extern void con_srvs_init(con_srvs_t *);

extern errno_t con_conn(ipc_call_t *, con_srvs_t *);

#endif

/** @}
 */
