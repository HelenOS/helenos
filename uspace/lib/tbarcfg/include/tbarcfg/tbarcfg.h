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

/** @addtogroup libtbarcfg
 * @{
 */
/**
 * @file Start menu
 */

#ifndef _TBARCFG_TBARCFG_H
#define _TBARCFG_TBARCFG_H

#include <errno.h>
#include <sif.h>
#include <stdbool.h>
#include <types/tbarcfg/tbarcfg.h>

#define TBARCFG_NOTIFY_DEFAULT "tbarcfg-notif"

extern errno_t tbarcfg_create(const char *, tbarcfg_t **);
extern errno_t tbarcfg_open(const char *, tbarcfg_t **);
extern void tbarcfg_close(tbarcfg_t *);
extern errno_t tbarcfg_sync(tbarcfg_t *);
extern smenu_entry_t *tbarcfg_smenu_first(tbarcfg_t *);
extern smenu_entry_t *tbarcfg_smenu_next(smenu_entry_t *);
extern smenu_entry_t *tbarcfg_smenu_last(tbarcfg_t *);
extern smenu_entry_t *tbarcfg_smenu_prev(smenu_entry_t *);
extern const char *smenu_entry_get_caption(smenu_entry_t *);
extern const char *smenu_entry_get_cmd(smenu_entry_t *);
extern bool smenu_entry_get_terminal(smenu_entry_t *);
extern bool smenu_entry_get_separator(smenu_entry_t *);
extern errno_t smenu_entry_set_caption(smenu_entry_t *, const char *);
extern errno_t smenu_entry_set_cmd(smenu_entry_t *, const char *);
extern void smenu_entry_set_terminal(smenu_entry_t *, bool);
extern errno_t smenu_entry_create(tbarcfg_t *, const char *, const char *,
    bool, smenu_entry_t **);
extern errno_t smenu_entry_sep_create(tbarcfg_t *, smenu_entry_t **);
extern void smenu_entry_destroy(smenu_entry_t *);
extern void smenu_entry_move_up(smenu_entry_t *);
extern void smenu_entry_move_down(smenu_entry_t *);
extern errno_t tbarcfg_listener_create(const char *, void (*)(void *),
    void *, tbarcfg_listener_t **);
extern void tbarcfg_listener_destroy(tbarcfg_listener_t *);
extern errno_t tbarcfg_notify(const char *);

#endif

/** @}
 */
