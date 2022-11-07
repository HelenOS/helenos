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

/** @addtogroup libwndmgt
 * @{
 */
/** @file
 */

#ifndef _LIBWNDMGT_WNDMGT_H_
#define _LIBWNDMGT_WNDMGT_H_

#include <errno.h>
#include <types/common.h>
#include "types/wndmgt.h"

extern errno_t wndmgt_open(const char *, wndmgt_cb_t *, void *, wndmgt_t **);
extern void wndmgt_close(wndmgt_t *);
extern errno_t wndmgt_get_window_list(wndmgt_t *, wndmgt_window_list_t **);
extern void wndmgt_free_window_list(wndmgt_window_list_t *);
extern errno_t wndmgt_get_window_info(wndmgt_t *, sysarg_t,
    wndmgt_window_info_t **);
extern void wndmgt_free_window_info(wndmgt_window_info_t *);
extern errno_t wndmgt_activate_window(wndmgt_t *, sysarg_t, sysarg_t);
extern errno_t wndmgt_close_window(wndmgt_t *, sysarg_t);

#endif

/** @}
 */
