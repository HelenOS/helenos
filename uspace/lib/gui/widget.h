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

#ifndef GUI_WIDGET_H_
#define GUI_WIDGET_H_

#include <adt/list.h>
#include <io/window.h>

struct window;
typedef struct window window_t;

struct widget;
typedef struct widget widget_t;

/**
 * Base class for all widgets. This structure should be first data member of
 * any derived widget structure.
 */
struct widget {
	link_t link;
	widget_t *parent;  /**< Parent widget of this widget. NULL for root widget. */
	list_t children;   /**< Children widgets of this widget. */
	window_t *window;  /**< Window into which this widget belongs. */
	const void *data;  /**< Custom client data. */

	sysarg_t hpos; /**< Horizontal position in window coordinates. */
	sysarg_t vpos; /**< Vertical position in window coordinates. */
	sysarg_t width;
	sysarg_t height;

	sysarg_t width_min;
	sysarg_t height_min;
	sysarg_t width_ideal; /**< Width size hint for initialization. */
	sysarg_t height_ideal; /**< Height size hint for initialization, */
	sysarg_t width_max;
	sysarg_t height_max;

	/**
	 * Virtual destructor. Apart from deallocating the resources specific for
	 * the particular widget, each widget shall remove itself from parents
	 * children and deallocate itself. */
	void (*destroy)(widget_t *);

	/**
	 * Reserved for bottom-top traversal when widget changes its properties and
	 * want to inform its ancestors in widget hierarchy to consider rearranging
	 * their children. As a reaction to this call, each widget shall fetch
	 * information from its children and decide whether its own properties have
	 * to be changed. If not, widget shall calculate new layout for its children
	 * and call rearrange() on each of them. Otherwise, widget shall change its
	 * own properties and call reconfigure() on its parent. */
	void (*reconfigure)(widget_t *);

	/**
	 * Reserved for top-bottom traversal when widget decides to change layout
	 * of its childer. As a reaction to this call, widget shall change its
	 * position and size according to provided arguments, paint itself,
	 * calculate new layout for its children and call rearrange() on each
	 * of them. */
	void (*rearrange)(widget_t *, sysarg_t, sysarg_t, sysarg_t, sysarg_t);

	/**
	 * As a reaction to window refresh event, widget hierarchy is traversed
	 * in top-bottom order and repaint() is called on each widget. Widget shall
	 * either paint itself or copy its private buffer onto window surface.
	 * Widget shall also post damage event into window event loop. */
	void (*repaint)(widget_t *);

	/**
	 * Keyboard events are delivered to widgets that have keyboard focus. As a
	 * reaction to the event, widget might call reconfigure() on its parent or
	 * rearrange() on its children. If the widget wants to change its visual
	 * information, refresh event should be posted to the window event loop. */
	void (*handle_keyboard_event)(widget_t *, kbd_event_t);

	/**
	 * Position events are delivered to those widgets that have mouse grab or
	 * those that intersects with cursor. As a reaction to the event, widget
	 * might call reconfigure() on its parent or rearrange() on its children.
	 * If the widget wants to change its visual information, refresh event
	 * should be posted to the window event loop. If the widget accepts
	 * keyboard events, it should take ownership of keyboard focus. Widget can
	 * also acquire or release mouse grab. */
	void (*handle_position_event)(widget_t *, pos_event_t);
};

/*
 * Note on derived widget constructor/destructor:
 * In order to support inheritance, wach widget shall provide init and deinit
 * function. These functions take already allocated widget and are responsible
 * for (de)initializing all widget-specific resources, inserting/removing the
 * widget into/from widget hierarchy and (de)initializing all data and functions
 * from the top-level base class defined above. For convenience, each widget
 * should also provide constructor to allocate and init the widget in one step.
 */

extern void widget_init(widget_t *, widget_t *, const void *);
extern void widget_modify(widget_t *, sysarg_t, sysarg_t, sysarg_t, sysarg_t);
extern const void *widget_get_data(widget_t *);
extern void widget_deinit(widget_t *);

#endif

/** @}
 */

