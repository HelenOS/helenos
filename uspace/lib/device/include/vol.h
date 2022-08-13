/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 */

#ifndef LIBDEVICE_VOL_H
#define LIBDEVICE_VOL_H

#include <async.h>
#include <errno.h>
#include <loc.h>
#include <stdint.h>
#include <types/label.h>
#include <types/vol.h>

extern errno_t vol_create(vol_t **);
extern void vol_destroy(vol_t *);
extern errno_t vol_get_parts(vol_t *, service_id_t **, size_t *);
extern errno_t vol_part_add(vol_t *, service_id_t);
extern errno_t vol_part_info(vol_t *, service_id_t, vol_part_info_t *);
extern errno_t vol_part_eject(vol_t *, service_id_t);
extern errno_t vol_part_empty(vol_t *, service_id_t);
extern errno_t vol_part_insert(vol_t *, service_id_t);
extern errno_t vol_part_insert_by_path(vol_t *, const char *);
extern errno_t vol_part_get_lsupp(vol_t *, vol_fstype_t, vol_label_supp_t *);
extern errno_t vol_part_mkfs(vol_t *, service_id_t, vol_fstype_t, const char *,
    const char *);
extern errno_t vol_part_set_mountp(vol_t *, service_id_t, const char *);
extern errno_t vol_get_volumes(vol_t *, volume_id_t **, size_t *);
extern errno_t vol_info(vol_t *, volume_id_t, vol_info_t *);
extern errno_t vol_fstype_format(vol_fstype_t, char **);
extern errno_t vol_pcnt_fs_format(vol_part_cnt_t, vol_fstype_t, char **);
extern errno_t vol_mountp_validate(const char *);

#endif

/** @}
 */
