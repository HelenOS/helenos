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

/** @addtogroup libdispcfg
 * @{
 */
/** @file
 */

#ifndef _LIBDISPCFG_DISPCFG_H_
#define _LIBDISPCFG_DISPCFG_H_

#include <errno.h>
#include <types/common.h>
#include "types/dispcfg.h"

extern errno_t dispcfg_open(const char *, dispcfg_cb_t *, void *, dispcfg_t **);
extern void dispcfg_close(dispcfg_t *);
extern errno_t dispcfg_get_seat_list(dispcfg_t *, dispcfg_seat_list_t **);
extern void dispcfg_free_seat_list(dispcfg_seat_list_t *);
extern errno_t dispcfg_get_seat_info(dispcfg_t *, sysarg_t,
    dispcfg_seat_info_t **);
extern void dispcfg_free_seat_info(dispcfg_seat_info_t *);
extern errno_t dispcfg_seat_create(dispcfg_t *, const char *, sysarg_t *);
extern errno_t dispcfg_seat_delete(dispcfg_t *, sysarg_t);
extern errno_t dispcfg_dev_assign(dispcfg_t *, sysarg_t, sysarg_t);
extern errno_t dispcfg_dev_unassign(dispcfg_t *, sysarg_t);
extern errno_t dispcfg_get_asgn_dev_list(dispcfg_t *, sysarg_t,
    dispcfg_dev_list_t **);
extern void dispcfg_free_dev_list(dispcfg_dev_list_t *);

#endif

/** @}
 */
