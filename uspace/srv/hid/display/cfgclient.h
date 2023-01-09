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

/** @addtogroup display
 * @{
 */
/**
 * @file Display server CFG client
 */

#ifndef CFGCLIENT_H
#define CFGCLIENT_H

#include <errno.h>
#include <dispcfg.h>
#include "types/display/display.h"
#include "types/display/cfgclient.h"

extern errno_t ds_cfgclient_create(ds_display_t *, ds_cfgclient_cb_t *, void *,
    ds_cfgclient_t **);
extern void ds_cfgclient_destroy(ds_cfgclient_t *);
extern errno_t ds_cfgclient_get_event(ds_cfgclient_t *, dispcfg_ev_t *);
extern errno_t ds_cfgclient_post_seat_added_event(ds_cfgclient_t *, sysarg_t);
extern errno_t ds_cfgclient_post_seat_removed_event(ds_cfgclient_t *, sysarg_t);
extern void ds_cfgclient_purge_events(ds_cfgclient_t *);

#endif

/** @}
 */
