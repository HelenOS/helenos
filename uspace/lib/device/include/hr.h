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

/** @addtogroup libdevice
 * @{
 */
/**
 * @file
 */

#ifndef LIBDEVICE_HR_H
#define LIBDEVICE_HR_H

#include <async.h>
#include <errno.h>
#include <loc.h>

/* for now */
#define HR_MAX_EXTENTS 4
#define HR_MAX_HOTSPARES HR_MAX_EXTENTS

#define HR_DEVNAME_LEN 32

typedef enum hr_level {
	HR_LVL_0 = 0x00, /* striping, no redundancy */
	HR_LVL_1 = 0x01, /* n-way mirroring */
	HR_LVL_4 = 0x04, /* dedicated parity */
	HR_LVL_5 = 0x05, /* distributed parity */
	HR_LVL_UNKNOWN = 0xFF
} hr_level_t;

typedef enum hr_layout {
	HR_LAYOUT_NONE = 0,
	HR_LAYOUT_RAID4_0, /* RAID-4 Non-Rotating Parity 0 */
	HR_LAYOUT_RAID4_N, /* RAID-4 Non-Rotating Parity N */
	HR_LAYOUT_RAID5_0R, /* RAID-5 Rotating Parity 0 with Data Restart */
	HR_LAYOUT_RAID5_NR, /* RAID-5 Rotating Parity N with Data Restart */
	HR_LAYOUT_RAID5_NC /* RAID-5 Rotating Parity N with Data Continuation */
} hr_layout_t;

typedef enum hr_vol_state {
	HR_VOL_NONE = 0, /* Unknown/None */
	HR_VOL_ONLINE, /* optimal */
	HR_VOL_FAULTY, /* unusable */
	HR_VOL_DEGRADED, /* not optimal */
	HR_VOL_REBUILD /* rebuild in progress */
} hr_vol_state_t;

typedef enum hr_ext_state {
	HR_EXT_NONE = 0, /* unknown/none state */
	HR_EXT_INVALID, /* working but not consistent */
	HR_EXT_ONLINE, /* ok */
	HR_EXT_MISSING, /* offline */
	HR_EXT_FAILED,
	HR_EXT_REBUILD,
	HR_EXT_HOTSPARE
} hr_ext_state_t;

typedef enum {
	HR_METADATA_NATIVE = 0,
	HR_METADATA_GEOM_MIRROR,
	HR_METADATA_GEOM_STRIPE,
	HR_METADATA_SOFTRAID,
	HR_METADATA_LAST_DUMMY
} hr_metadata_type_t;

typedef struct hr {
	async_sess_t *sess;
} hr_t;

typedef struct hr_config {
	char devname[HR_DEVNAME_LEN];
	service_id_t devs[HR_MAX_EXTENTS];
	size_t dev_no;
	hr_level_t level;
} hr_config_t;

typedef struct hr_extent {
	service_id_t svc_id;
	hr_ext_state_t state;
} hr_extent_t;

typedef struct hr_pair_vol_state {
	service_id_t svc_id;
	hr_vol_state_t state;
} hr_pair_vol_state_t;

typedef struct hr_vol_info {
	char devname[HR_DEVNAME_LEN];
	service_id_t svc_id;
	hr_level_t level;
	hr_extent_t extents[HR_MAX_EXTENTS];
	hr_extent_t hotspares[HR_MAX_HOTSPARES];
	size_t extent_no;
	size_t hotspare_no;
	uint64_t data_blkno;
	uint64_t rebuild_blk;
	uint32_t strip_size;
	size_t bsize;
	hr_vol_state_t state;
	hr_layout_t layout;
	hr_metadata_type_t meta_type;
	/* TODO: add rebuild pos */
} hr_vol_info_t;

extern errno_t hr_sess_init(hr_t **);
extern void hr_sess_destroy(hr_t *);
extern errno_t hr_create(hr_t *, hr_config_t *);
extern errno_t hr_assemble(hr_t *, hr_config_t *, size_t *);
extern errno_t hr_auto_assemble(hr_t *, size_t *);
extern errno_t hr_stop(hr_t *, const char *);
extern errno_t hr_stop_all(hr_t *);
extern errno_t hr_fail_extent(hr_t *, const char *, unsigned long);
extern errno_t hr_add_hotspare(hr_t *, const char *, const char *);
extern errno_t hr_get_vol_states(hr_t *, hr_pair_vol_state_t **, size_t *);
extern errno_t hr_get_vol_info(hr_t *, service_id_t, hr_vol_info_t *);
extern const char *hr_get_vol_state_str(hr_vol_state_t);
extern const char *hr_get_ext_state_str(hr_ext_state_t);
extern const char *hr_get_layout_str(hr_layout_t);
extern const char *hr_get_level_str(hr_level_t);
extern const char *hr_get_metadata_type_str(hr_metadata_type_t);

#endif

/** @}
 */
