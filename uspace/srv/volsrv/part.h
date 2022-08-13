/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup volsrv
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef PART_H_
#define PART_H_

#include <loc.h>
#include <stddef.h>
#include <types/vol.h>
#include "types/part.h"
#include "types/volume.h"

extern errno_t vol_parts_create(vol_volumes_t *, vol_parts_t **);
extern void vol_parts_destroy(vol_parts_t *);
extern errno_t vol_part_discovery_start(vol_parts_t *);
extern errno_t vol_part_add_part(vol_parts_t *, service_id_t);
extern errno_t vol_part_get_ids(vol_parts_t *, service_id_t *, size_t,
    size_t *);
extern errno_t vol_part_find_by_id_ref(vol_parts_t *, service_id_t,
    vol_part_t **);
extern errno_t vol_part_find_by_path_ref(vol_parts_t *, const char *,
    vol_part_t **);
extern void vol_part_del_ref(vol_part_t *);
extern errno_t vol_part_eject_part(vol_part_t *);
extern errno_t vol_part_empty_part(vol_part_t *);
extern errno_t vol_part_insert_part(vol_part_t *);
extern errno_t vol_part_mkfs_part(vol_part_t *, vol_fstype_t, const char *,
    const char *);
extern errno_t vol_part_set_mountp_part(vol_part_t *, const char *);
extern errno_t vol_part_get_info(vol_part_t *, vol_part_info_t *);

#endif

/** @}
 */
