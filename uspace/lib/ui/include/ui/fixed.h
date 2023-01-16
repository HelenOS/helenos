/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** @addtogroup libui
 * @{
 */
/**
 * @file Fixed layout
 */

#ifndef _UI_FIXED_H
#define _UI_FIXED_H

#include <errno.h>
#include <io/pos_event.h>
#include <types/ui/control.h>
#include <types/ui/event.h>
#include <types/ui/fixed.h>

extern errno_t ui_fixed_create(ui_fixed_t **);
extern void ui_fixed_destroy(ui_fixed_t *);
extern ui_control_t *ui_fixed_ctl(ui_fixed_t *);
extern errno_t ui_fixed_add(ui_fixed_t *, ui_control_t *);
extern void ui_fixed_remove(ui_fixed_t *, ui_control_t *);
extern errno_t ui_fixed_paint(ui_fixed_t *);
extern ui_evclaim_t ui_fixed_kbd_event(ui_fixed_t *, kbd_event_t *);
extern ui_evclaim_t ui_fixed_pos_event(ui_fixed_t *, pos_event_t *);
extern void ui_fixed_unfocus(ui_fixed_t *, unsigned);

#endif

/** @}
 */
