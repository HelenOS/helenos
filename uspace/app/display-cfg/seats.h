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

/** @addtogroup display-cfg
 * @{
 */
/**
 * @file Seat configuration tab
 */

#ifndef SEATS_H
#define SEATS_H

#include "types/display-cfg.h"
#include "types/seats.h"

extern errno_t dcfg_seats_create(display_cfg_t *, dcfg_seats_t **);
extern errno_t dcfg_seats_populate(dcfg_seats_t *);
extern void dcfg_seats_destroy(dcfg_seats_t *);
extern errno_t dcfg_seats_insert(dcfg_seats_t *, const char *, sysarg_t,
    dcfg_seats_entry_t **);
extern errno_t dcfg_devices_insert(dcfg_seats_t *, const char *,
    service_id_t);
extern errno_t dcfg_avail_devices_insert(dcfg_seats_t *seats,
    ui_select_dialog_t *dialog, const char *name, service_id_t svc_id);
extern errno_t dcfg_seats_list_populate(dcfg_seats_t *);

#endif

/** @}
 */
