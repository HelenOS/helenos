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

#ifndef GUI_WINDOW_H_
#define GUI_WINDOW_H_

#include <async.h>
#include <adt/prodcons.h>
#include <fibril_synch.h>
#include <ipc/window.h>
#include <io/window.h>
#include <surface.h>

#include "widget.h"

struct window {
	bool is_main; /**< True for the main window of the application. */
	bool is_decorated; /**< True if the window decorations should be rendered. */
	bool is_focused; /**< True for the top level window of the desktop. */
	char *caption; /**< Text title of the window header. */
	async_sess_t *isess; /**< Input events from compositor. */
	async_sess_t *osess; /**< Mainly for damage reporting to compositor. */
	prodcons_t events; /**< Queue for window event loop. */
	widget_t root; /**< Decoration widget serving as a root of widget hiearchy. */
	widget_t *grab; /**< Widget owning the mouse or NULL. */
	widget_t *focus; /**< Widget owning the keyboard or NULL. */
	fibril_mutex_t guard; /**< Mutex guarding window surface. */
	surface_t *surface; /**< Window surface shared with compositor. */
};

/**
 * Allocate all resources for new window and register it in the compositor.
 * If the window is declared as main, its closure causes termination of the
 * whole application. Note that opened window does not have any surface yet.
 */
extern window_t *window_open(const char *, const void *, window_flags_t,
    const char *);

/**
 * Post resize event into event loop. Window negotiates new surface with
 * compositor and asks all widgets in the tree to calculate their new properties
 * and to paint themselves on the new surface (top-bottom order). Should be
 * called also after opening new window to obtain surface. */
extern void window_resize(window_t *, sysarg_t, sysarg_t, sysarg_t, sysarg_t,
    window_placement_flags_t);

/** Change window caption. */
extern errno_t window_set_caption(window_t *, const char *);

/**
 * Post refresh event into event loop. Widget tree is traversed and all widgets
 * are asked to repaint themselves in top-bottom order. Should be called by
 * widget after such change of its internal state that does not need resizing
 * of neither parent nor children (). */
extern void window_refresh(window_t *);

/**
 * Post damage event into event loop. Handler informs compositor to update the
 * window surface on the screen. Should be called by widget after painting
 * itself or copying its buffer onto window surface. */
extern void window_damage(window_t *);

/**
 * Retrieve dummy root widget of the window widget tree. Intended to be called
 * by proper top-level widget to set his parent. */
extern widget_t *window_root(window_t *);

/**
 * Prepare and enqueue window fibrils for event loop and input fetching. When
 * async_manager() function is called, event loop is executed. */
extern void window_exec(window_t *);

/**
 * Claim protected window surface. Intended for widgets painting from their
 * internal fibrils (e.g. terminal, animation, video). */
extern surface_t *window_claim(window_t *);

/**
 * Yield protected window surface after painting. */
extern void window_yield(window_t *);

/**
 * Initiate the closing cascade for the window. First, compositor deallocates
 * output resources, prepares special closing input event for the window and
 * deallocates input resources after the event is dispatched. When window
 * fetches closing event, it is posted into event loop and input fibril
 * terminates. When event loop fibril handles closing event, all window
 * resources are deallocated and fibril also terminates. Moreover, if the
 * window is main window of the application, whole task terminates. */
extern void window_close(window_t *);

#endif

/** @}
 */
