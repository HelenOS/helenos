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

/** @addtogroup taskbar
 * @{
 */
/**
 * @file Taskbar clock
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <errno.h>
#include <gfx/coord.h>
#include <io/kbd_event.h>
#include <io/pos_event.h>
#include <ui/control.h>
#include <ui/window.h>
#include <stdbool.h>
#include "types/clock.h"

extern errno_t taskbar_clock_create(ui_window_t *, taskbar_clock_t **);
extern void taskbar_clock_destroy(taskbar_clock_t *);
extern errno_t taskbar_clock_paint(taskbar_clock_t *);
extern ui_evclaim_t taskbar_clock_kbd_event(taskbar_clock_t *, kbd_event_t *);
extern ui_evclaim_t taskbar_clock_pos_event(taskbar_clock_t *, pos_event_t *);
extern ui_control_t *taskbar_clock_ctl(taskbar_clock_t *);
extern void taskbar_clock_set_rect(taskbar_clock_t *, gfx_rect_t *);

#endif

/** @}
 */
