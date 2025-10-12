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

#include <adt/list.h>
#include <errno.h>
#include <hr.h>
#include <io/log.h>

#include "superblock.h"
#include "var.h"

#define HR_DEBUG(format, ...) \
    log_msg(LOG_DEFAULT, LVL_DEBUG, format, ##__VA_ARGS__)

#define HR_NOTE(format, ...) \
    log_msg(LOG_DEFAULT, LVL_NOTE, format, ##__VA_ARGS__)

#define HR_WARN(format, ...) \
    log_msg(LOG_DEFAULT, LVL_WARN, format, ##__VA_ARGS__)

#define HR_ERROR(format, ...) \
    log_msg(LOG_DEFAULT, LVL_ERROR, format, ##__VA_ARGS__)

struct dev_list_member {
	link_t link;
	service_id_t svc_id;
	void *md;
	bool inited;
	bool md_present;
	bool fini;
};

typedef struct hr_range_lock {
	link_t link;
	fibril_mutex_t lock;
	hr_volume_t *vol; /* back-pointer to volume */
	uint64_t off; /* start of the range */
	uint64_t len; /* length of the range */

	size_t pending; /* prot. by vol->range_lock_list_lock */
	bool ignore; /* prot. by vol->range_lock_list_lock */
} hr_range_lock_t;

extern void *hr_malloc_waitok(size_t)
    __attribute__((malloc));

extern void *hr_calloc_waitok(size_t, size_t)
    __attribute__((malloc));

extern errno_t hr_create_vol_struct(hr_volume_t **, hr_level_t, const char *,
    hr_metadata_type_t, uint8_t);
extern void hr_destroy_vol_struct(hr_volume_t *);
extern errno_t hr_get_volume_svcs(size_t *, service_id_t **);
extern hr_volume_t *hr_get_volume(service_id_t);
extern errno_t hr_remove_volume(service_id_t);
extern errno_t hr_init_extents_from_cfg(hr_volume_t *, hr_config_t *);
extern void hr_fini_devs(hr_volume_t *);
extern errno_t hr_register_volume(hr_volume_t *);
extern errno_t hr_check_ba_range(hr_volume_t *, size_t, uint64_t);
extern void hr_add_data_offset(hr_volume_t *, uint64_t *);
extern void hr_sub_data_offset(hr_volume_t *, uint64_t *);
extern void hr_update_ext_state(hr_volume_t *, size_t, hr_ext_state_t);
extern void hr_update_hotspare_state(hr_volume_t *, size_t, hr_ext_state_t);
extern void hr_update_vol_state(hr_volume_t *, hr_vol_state_t);
extern void hr_update_ext_svc_id(hr_volume_t *, size_t, service_id_t);
extern void hr_update_hotspare_svc_id(hr_volume_t *, size_t, service_id_t);
extern void hr_sync_all_extents(hr_volume_t *);
extern size_t hr_count_extents(hr_volume_t *, hr_ext_state_t);
extern void hr_mark_vol_state_dirty(hr_volume_t *);
extern hr_range_lock_t *hr_range_lock_acquire(hr_volume_t *, uint64_t,
    uint64_t);
extern void hr_range_lock_release(hr_range_lock_t *);
extern errno_t hr_util_try_assemble(hr_config_t *, size_t *);
extern errno_t hr_util_add_hotspare(hr_volume_t *, service_id_t);
extern void hr_raid5_xor(void *, const void *, size_t);
extern errno_t hr_sync_extents(hr_volume_t *);
extern errno_t hr_init_rebuild(hr_volume_t *, size_t *);
extern uint32_t hr_closest_pow2(uint32_t);

#endif

/** @}
 */
