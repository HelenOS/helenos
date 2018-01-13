/*
 * Copyright (c) 2012 Petr Koupy
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

/** @addtogroup gui
 * @{
 */
/**
 * @file
 */

#ifndef GUI_TERMINAL_H_
#define GUI_TERMINAL_H_

#include <stddef.h>
#include <fibril_synch.h>
#include <font.h>
#include <io/chargrid.h>
#include <io/con_srv.h>
#include <adt/list.h>
#include <adt/prodcons.h>
#include <atomic.h>
#include <str.h>
#include "widget.h"

#define UTF8_CHAR_BUFFER_SIZE  (STR_BOUNDS(1) + 1)

typedef struct terminal {
	widget_t widget;
	
	fibril_mutex_t mtx;
	link_t link;
	atomic_t refcnt;
	
	prodcons_t input_pc;
	char char_remains[UTF8_CHAR_BUFFER_SIZE];
	size_t char_remains_len;
	
	sysarg_t cols;
	sysarg_t rows;
	chargrid_t *frontbuf;
	chargrid_t *backbuf;
	sysarg_t top_row;
	
	service_id_t dsid;
	con_srvs_t srvs;
} terminal_t;

extern bool init_terminal(terminal_t *, widget_t *, const void *, sysarg_t,
    sysarg_t);
extern terminal_t *create_terminal(widget_t *, const void *, sysarg_t,
    sysarg_t);
extern void deinit_terminal(terminal_t *);

#endif

/** @}
 */
