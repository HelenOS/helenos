/*
 * Copyright (c) 2025 Miroslav Cimerman
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

/** @addtogroup hr
 * @{
 */
/**
 * @file
 */

#ifndef _HR_UTIL_H
#define _HR_UTIL_H

#include <errno.h>
#include <io/log.h>

#include "var.h"

#define HR_DEBUG(format, ...) \
    log_msg(LOG_DEFAULT, LVL_DEBUG, format, ##__VA_ARGS__)

#define HR_WARN(format, ...) \
    log_msg(LOG_DEFAULT, LVL_WARN, format, ##__VA_ARGS__)

#define HR_ERROR(format, ...) \
    log_msg(LOG_DEFAULT, LVL_ERROR, format, ##__VA_ARGS__)


extern errno_t		 hr_create_vol_struct(hr_volume_t **, hr_level_t);
extern void		 hr_destroy_vol_struct(hr_volume_t *);
extern hr_volume_t	*hr_get_volume(service_id_t);
extern errno_t		 hr_remove_volume(service_id_t);
extern errno_t		 hr_init_devs(hr_volume_t *);
extern void		 hr_fini_devs(hr_volume_t *);
extern errno_t		 hr_register_volume(hr_volume_t *);
extern errno_t		 hr_check_devs(hr_volume_t *, uint64_t *, size_t *);
extern errno_t		 hr_check_ba_range(hr_volume_t *, size_t, uint64_t);
extern void		 hr_add_ba_offset(hr_volume_t *, uint64_t *);
extern void		 hr_update_ext_status(hr_volume_t *, size_t,
    hr_ext_status_t);
extern void		 hr_update_hotspare_status(hr_volume_t *, size_t,
    hr_ext_status_t);
extern void		 hr_update_vol_status(hr_volume_t *, hr_vol_status_t);
extern void		 hr_update_ext_svc_id(hr_volume_t *, size_t,
    service_id_t);
extern void		 hr_update_hotspare_svc_id(hr_volume_t *, size_t,
    service_id_t);
extern void		 hr_sync_all_extents(hr_volume_t *);
extern size_t		 hr_count_extents(hr_volume_t *, hr_ext_status_t);
extern void		 hr_mark_vol_state_dirty(hr_volume_t *);
extern void		 hr_range_lock_release(hr_range_lock_t *);
extern hr_range_lock_t	*hr_range_lock_acquire(hr_volume_t *, uint64_t,
    uint64_t);
extern errno_t		 hr_util_try_auto_assemble(size_t *);

#endif

/** @}
 */
