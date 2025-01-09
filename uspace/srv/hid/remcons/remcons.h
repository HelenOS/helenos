/*
 * Copyright (c) 2024 Jiri Svoboda
 * Copyright (c) 2012 Vojtech Horky
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

/** @addtogroup remcons
 * @{
 */
/** @file
 */

#ifndef REMCONS_H_
#define REMCONS_H_

#include <adt/list.h>
#include <io/cons_event.h>
#include <stdbool.h>
#include <vt/vt100.h>
#include "user.h"

#define NAME       "remcons"
#define NAMESPACE  "term"

/** Remote console */
typedef struct {
	telnet_user_t *user;	/**< telnet user */
	vt100_t *vt;		/**< virtual terminal driver */
	bool enable_ctl;	/**< enable escape control sequences */
	bool enable_rgb;	/**< enable RGB color setting */
	sysarg_t ucols;		/**< number of columns in user buffer */
	sysarg_t urows;		/**< number of rows in user buffer */
	charfield_t *ubuf;	/**< user buffer */
	bool curs_visible;	/**< cursor is visible */

	/** List of remcons_event_t. */
	list_t in_events;
} remcons_t;

/** Remote console event */
typedef struct {
	link_t link;		/**< link to list of events */
	cons_event_t cev;	/**< console event */
} remcons_event_t;

#endif

/**
 * @}
 */
