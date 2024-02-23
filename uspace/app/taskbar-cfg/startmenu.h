/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup taskbar-cfg
 * @{
 */
/**
 * @file Start menu configuration tab
 */

#ifndef STARTMENU_H
#define STARTMENU_H

#include <tbarcfg/tbarcfg.h>
#include "types/startmenu.h"
#include "types/taskbar-cfg.h"

extern errno_t startmenu_create(taskbar_cfg_t *, startmenu_t **);
extern errno_t startmenu_populate(startmenu_t *, tbarcfg_t *);
extern void startmenu_destroy(startmenu_t *);
extern errno_t startmenu_insert(startmenu_t *, smenu_entry_t *,
    startmenu_entry_t **);
extern startmenu_entry_t *startmenu_get_selected(startmenu_t *);
extern void startmenu_new_entry(startmenu_t *);
extern void startmenu_sep_entry(startmenu_t *);
extern void startmenu_edit(startmenu_t *);
extern errno_t startmenu_entry_update(startmenu_entry_t *);
extern void startmenu_repaint(startmenu_t *);

#endif

/** @}
 */
